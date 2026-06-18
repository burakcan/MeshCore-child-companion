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
static bool pollKeySdl() {
  sdlPumpEvents();
  if (sdlQuitRequested()) return false;
  int c;
  while ((c = sdlReadByte()) >= 0) {
    if (c == 0x1b) { int a = sdlReadByte(), b = sdlReadByte(); if (a == '[' && b >= 0) applyArrow((char)b); }
    else if (!applyKey((unsigned char)c)) return false;
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
