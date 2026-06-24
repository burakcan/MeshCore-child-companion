// Phase 3: the REAL companion firmware WITH the real ui-new UITask + child mode,
// rendering to the terminal via HostDisplay, driven by the PC keyboard. This is
// the genuine UI on the genuine mesh stack (not the thin harness's mirrors).
#include <Arduino.h>
#include <target.h>
#include <helpers/ArduinoHelpers.h>
#include <helpers/SimpleMeshTables.h>
#include <helpers/ArduinoSerialInterface.h>
#include "HostFS.h"
#include "MyMesh.h"
#include "UITask.h"                         // examples/companion_radio/ui-new
#ifdef CHILD_MODE
#include "child_mode/ChildMode.h"
#endif
#include "../host_display.h"                // terminal renderer (thin-harness HostDisplay)
#ifdef USE_SDL
#include "sdl_backend.h"                    // optional SDL2 graphic window
#endif

#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#ifdef CHILD_MODE
#include <helpers/ContactInfo.h>
#include <helpers/AdvertDataHelpers.h>   // ADV_TYPE_CHAT
#include <helpers/TxtDataHelpers.h>      // TXT_TYPE_PLAIN
#endif

class NullStream : public Stream {
public: size_t write(uint8_t) override { return 1; } using Print::write;
};

// Globals named exactly as the firmware expects (child_mode/main reference them).
static NullStream       nullStream;
ArduinoSerialInterface  serial_interface;
UITask                  ui_task(&board, &serial_interface);
HostFS                  InternalFS;
StdRNG                  fast_rng;
SimpleMeshTables        tables;
DataStore               store(InternalFS, rtc_clock);
MyMesh                  the_mesh(radio_driver, fast_rng, rtc_clock, tables, store, &ui_task);
static HostDisplay      display;

static struct termios s_orig;
static void rawBegin() {
  tcgetattr(STDIN_FILENO, &s_orig);
  struct termios r = s_orig; r.c_lflag &= ~(ICANON | ECHO); r.c_cc[VMIN] = 0; r.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &r);
  fputs("\x1b[2J", stdout);
}
static void rawEnd() { tcsetattr(STDIN_FILENO, TCSANOW, &s_orig); }

// Map an arrow ('A'up 'B'down 'C'right 'D'left) onto joystick pins.
static void applyArrow(char dir) {
#if defined(UI_HAS_JOYSTICK)
  if (dir == 'D') hostPressPin(JOYSTICK_LEFT);
  else if (dir == 'C') hostPressPin(JOYSTICK_RIGHT);
#if defined(UI_HAS_JOYSTICK_UPDOWN)
  else if (dir == 'A') hostPressPin(JOYSTICK_UP);
  else if (dir == 'B') hostPressPin(JOYSTICK_DOWN);
#else
  else if (dir == 'A') hostPressPin(JOYSTICK_LEFT);    // up ~ left (no up/down build)
  else if (dir == 'B') hostPressPin(JOYSTICK_RIGHT);   // down ~ right
#endif
#else
  (void)dir;
#endif
}

// Map a key byte onto a button pin. Returns false to quit.
static bool applyKey(unsigned char c) {
#if defined(UI_HAS_JOYSTICK)
  switch (c) {
    case '\r': case '\n': hostPressPin(PIN_USER_BTN, 120); break;   // press -> KEY_ENTER
    case 'a': hostPressPin(JOYSTICK_LEFT, 120); break;
    case 'd': hostPressPin(JOYSTICK_RIGHT, 120); break;
    case 'b': hostPressPin(PIN_BACK_BTN, 120); break;
    case 'q': case 'Q': return false;
  }
#else
  // Single-button (Heltec V3): tap=click, double/triple=prev/select, l=long press.
  switch (c) {
    case '\r': case '\n': hostPressPin(PIN_USER_BTN, 90); break;
    case 'l': case 'L':   hostPressPin(PIN_USER_BTN, 1300); break;
    case 'q': case 'Q':   return false;
  }
#endif
  return true;
}

static bool pollKeyTerminal() {
  fd_set fds; FD_ZERO(&fds); FD_SET(STDIN_FILENO, &fds);
  struct timeval tv{0, 0};
  if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) <= 0) return true;
  unsigned char c; if (read(STDIN_FILENO, &c, 1) != 1) return true;
  if (c == 0x1b) {                              // arrow escape sequence
    unsigned char b[2];
    if (read(STDIN_FILENO, &b[0], 1) == 1 && b[0] == '[' && read(STDIN_FILENO, &b[1], 1) == 1)
      applyArrow((char)b[1]);
    return true;
  }
  return applyKey(c);
}

#ifdef USE_SDL
static bool g_sdl = false;
// SDL gives real key down/up, so we hold the button pin for the actual hold
// duration: a tap = click, a hold (>1s) = long-press, just like the hardware.
static uint32_t pinForKey(int id) {
  switch (id) {
    case SK_PRESS: return PIN_USER_BTN;
#if defined(UI_HAS_JOYSTICK)
    case SK_LEFT:  return JOYSTICK_LEFT;
    case SK_RIGHT: return JOYSTICK_RIGHT;
    case SK_BACK:  return PIN_BACK_BTN;
#if defined(UI_HAS_JOYSTICK_UPDOWN)
    case SK_UP:    return JOYSTICK_UP;
    case SK_DOWN:  return JOYSTICK_DOWN;
#else
    case SK_UP:    return JOYSTICK_LEFT;
    case SK_DOWN:  return JOYSTICK_RIGHT;
#endif
#endif
    default: return 0xFFFFFFFFu;   // not mapped on this device
  }
}
static bool pollKeySdl() {
  sdlPumpEvents();
  if (sdlQuitRequested()) return false;
  int id; bool down;
  while (sdlNextKey(&id, &down)) {
    if (id == SK_QUIT) return false;
    uint32_t pin = pinForKey(id);
    if (pin != 0xFFFFFFFFu) {
      if (down) hostHoldPin(pin);
      else hostPressPin(pin, 40);   // keep pressed ~40ms after release so a fast tap still registers
    }
  }
  return true;
}
#endif

static bool pollKey() {
#ifdef USE_SDL
  if (g_sdl) return pollKeySdl();
#endif
  return pollKeyTerminal();
}

// ---------------------------------------------------------------------------
// Scripted screenshot/demo mode (SIM_SCRIPT=... ./meshnode-companion-ui).
// Injects mock messages through the REAL child capture path and drives the REAL
// UI via the same button bridge the keyboard uses, dumping each screen to a
// styled BMP. Not firmware; build-/README-tooling only. Inert unless SIM_SCRIPT set.
// ---------------------------------------------------------------------------
static void writeLE(FILE* f, uint32_t v, int n) { for (int i = 0; i < n; i++) { fputc(v & 0xFF, f); v >>= 8; } }

// 1-bit WxH framebuffer -> upscaled 24-bit BMP with an OLED look (lit pixels on a
// dark padded bezel). BMP needs no external lib; convert to PNG with `sips` after.
static void dumpBMP(const char* path, const uint8_t* fb, int W, int H, int scale, int pad) {
  int ow = W * scale + pad * 2, oh = H * scale + pad * 2;
  int rowbytes = (ow * 3 + 3) & ~3;
  uint32_t imgsize = (uint32_t)rowbytes * oh;
  FILE* f = fopen(path, "wb");
  if (!f) { fprintf(stderr, "dumpBMP: cannot open %s\n", path); return; }
  fputc('B', f); fputc('M', f);
  writeLE(f, 14 + 40 + imgsize, 4); writeLE(f, 0, 4); writeLE(f, 14 + 40, 4);   // file hdr
  writeLE(f, 40, 4); writeLE(f, ow, 4); writeLE(f, oh, 4);                       // DIB hdr
  writeLE(f, 1, 2); writeLE(f, 24, 2); writeLE(f, 0, 4); writeLE(f, imgsize, 4);
  writeLE(f, 2835, 4); writeLE(f, 2835, 4); writeLE(f, 0, 4); writeLE(f, 0, 4);
  const uint8_t litR = 196, litG = 240, litB = 255, bgR = 14, bgG = 18, bgB = 32;
  std::vector<uint8_t> row(rowbytes, 0);
  for (int oy = oh - 1; oy >= 0; oy--) {                       // BMP rows are bottom-up
    std::fill(row.begin(), row.end(), 0);
    for (int ox = 0; ox < ow; ox++) {
      int px = ox - pad, py = oy - pad;
      bool lit = (px >= 0 && py >= 0 && px < W * scale && py < H * scale) && fb[(py / scale) * W + (px / scale)];
      row[ox * 3 + 0] = lit ? litB : bgB;
      row[ox * 3 + 1] = lit ? litG : bgG;
      row[ox * 3 + 2] = lit ? litR : bgR;
    }
    fwrite(row.data(), 1, rowbytes, f);
  }
  fclose(f);
}

#ifdef CHILD_MODE
static uint32_t g_ts = 100000;   // sender timestamps; must increase to dodge dedup
static int g_dm = 0, g_ch = 0;

static ContactInfo mockContact(const char* name, uint8_t seed) {
  ContactInfo c; memset(&c, 0, sizeof(c));
  strncpy(c.name, name, sizeof(c.name) - 1);
  c.type = ADV_TYPE_CHAT;                                      // non-zero => approved sender
  for (int i = 0; i < 32; i++) c.id.pub_key[i] = (uint8_t)(seed + i);
  return c;
}
static void injectDM() {
  static const char* who[]  = { "Mom", "Dad", "Mom", "Grandpa" };
  static const char* body[] = { "Are you on your way home?",
                                "Dinner is ready - come home soon!",
                                "How was school today?",
                                "Want to go fishing on Saturday?" };
  int i = g_dm++ % 4;
  ContactInfo c = mockContact(who[i], (uint8_t)(0x10 + i * 0x21));
  child_mode.onIncomingText(c, TXT_TYPE_PLAIN, g_ts++, body[i]);
}
static void injectChannel() {
  static const char* body[] = { "Grandma: Call me when you can!",
                                "Dad: Family movie at 7pm",
                                "Mom: Who wants ice cream?" };
  child_mode.onIncomingChannel(0, "Family", g_ts++, body[g_ch++ % 3]);
}
static void injectQuestion() {
  ContactInfo c = mockContact("Mom", 0x55);
  child_mode.onIncomingText(c, TXT_TYPE_PLAIN, g_ts++, "?Dinner tonight | Pizza | Pasta");
}
static void injectBell() {
  ContactInfo c = mockContact("Dad", 0x77);
  child_mode.onIncomingText(c, TXT_TYPE_PLAIN, g_ts++, "\xF0\x9F\x94\x94");   // bare bell emoji
}
#endif

static void simPump(int ms) {                                 // run the real loops for ms real-time
  uint32_t end = millis() + (uint32_t)ms;
  while ((int32_t)(millis() - end) < 0) {
    ui_task.loop();
    the_mesh.loop();
    rtc_clock.tick();
    usleep(5000);
  }
}
static void simPress(uint32_t pin) { hostPressPin(pin, 130); simPump(280); }

// Returns true if it ran a script (caller should exit). 'display' must be begun.
static bool runScript() {
  const char* script = getenv("SIM_SCRIPT");
  if (!script) return false;
  const char* dir = getenv("SIM_SHOT_DIR"); if (!dir) dir = "sim/shots";
  display.setSilent(true);                                    // no terminal spam; we read the fb directly
  int shot = 0;
  simPump(500);                                               // let boot/splash settle
  for (const char* p = script; *p; p++) {
    switch (*p) {
      case 'c': simPress(PIN_USER_BTN);   break;              // center press / select
      case 'u': simPress(JOYSTICK_UP);    break;
      case 'd': simPress(JOYSTICK_DOWN);  break;
      case 'L': simPress(JOYSTICK_LEFT);  break;
      case 'R': simPress(JOYSTICK_RIGHT); break;
      case 'b': simPress(PIN_BACK_BTN);   break;              // back / menu button
#ifdef CHILD_MODE
      case 'M': injectDM();      simPump(550); break;
      case 'C': injectChannel(); simPump(550); break;
      case 'Q': injectQuestion();simPump(550); break;
      case 'B': injectBell();    simPump(550); break;
      case 'X': child_mode.openMessages(); simPump(400); break;
      case 'm': child_mode.openMenu();     simPump(400); break;
#endif
      case ',': simPump(400); break;                          // settle
      case '.': {                                             // capture
        char path[256];
        snprintf(path, sizeof(path), "%s/shot-%02d.bmp", dir, shot++);
        dumpBMP(path, display.framebuffer(), display.width(), display.height(), 5, 16);
        fprintf(stderr, "shot -> %s\n", path);
        break;
      }
      default: break;                                         // spaces / unknown = no-op
    }
  }
  return true;
}

int main() {
  Serial.begin(115200);
  fast_rng.begin(radio_driver.getRngSeed());
  InternalFS.begin();
  store.begin();
  the_mesh.begin(true);                       // has_display = true
  serial_interface.begin(nullStream);
  the_mesh.startInterface(serial_interface);
  sensors.begin();
#ifdef CHILD_MODE
  child_mode.begin();                         // must precede ui_task.begin (sets initial screen)
#endif
  ui_task.begin(&display, &sensors, the_mesh.getNodePrefs());

  if (runScript()) { fputs("\nscript done.\n", stderr); return 0; }   // SIM_SCRIPT screenshot mode

  radio_driver.enableUdpEther();              // reachable by ./meshnode-panel in another terminal

#ifdef USE_SDL
  const char* dm = getenv("SIM_DISPLAY");
  if (dm && !strcmp(dm, "sdl") && sdlBegin("MeshCore sim", display.width(), display.height(), 6)) {
    g_sdl = true; display.setSilent(true);    // present via the window, not the terminal
  }
  if (!g_sdl) rawBegin();
#else
  rawBegin();
#endif

  bool running = true;
  int tick = 0;
  while (running) {
    running = pollKey();
    ui_task.loop();                           // renders + reads buttons every ~8ms
    if (++tick % 4 == 0) {                     // mesh stack less often (it may block)
      the_mesh.loop();
      sensors.loop();
      rtc_clock.tick();
    }
#ifdef USE_SDL
    if (g_sdl) sdlPresent(display.framebuffer(), display.width(), display.height());
#endif
    usleep(8000);
  }

#ifdef USE_SDL
  if (g_sdl) { sdlEnd(); } else rawEnd();
#else
  rawEnd();
#endif
  fputs("\nbye.\n", stdout);
  return 0;
}
