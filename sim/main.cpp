// PC simulator for the MeshCore child-mode UI (Wio Tracker L1). Builds the real
// Arduino-free firmware code (widgets, child/ logic, ChildMessageScreen) and
// drives it from a host DisplayDriver (terminal) + keyboard.
//
// Caveat: the screen-flow glue below is a hand copy of
// examples/companion_radio/child_mode/ChildMode.cpp and the home screen copies
// ChildHomeScreen.cpp, because those pull in MyMesh.h / UITask.h and won't
// compile here. If the flow or home layout changes there, update it here too.

#include <helpers/ui/ListMenuScreen.h>
#include <helpers/ui/PinEntryScreen.h>
#include <helpers/ui/StatusHeader.h>
#include <helpers/ui/BatteryUtils.h>
#include <helpers/child/ChildConfig.h>
#include <helpers/child/ChildCommands.h>
#include <helpers/child/ChildMessageStore.h>
#include <ChildMessageScreen.h>   // real reader (via -I examples/companion_radio/child_mode)

#include "host_display.h"
#include "host_input.h"
#include "host_buzzer.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#ifndef CHILD_DEFAULT_PIN
#define CHILD_DEFAULT_PIN 1234
#endif

static const char* CFG_PATH = "sim/child_cfg.bin";
static const char* NODE_NAME = "Mesh";
static const uint16_t FAKE_BATT_MV = 4100;

// Demo payloads for the inject keys (long enough to exercise word-wrap/scroll).
static const char* DEMO_DM   = "Hi sweetie! Please come home by six oclock for dinner, ok? Love you lots, Mom";
static const char* DEMO_CHAN = "Soccer practice is moved to 5pm today at the north field. Bring water!";

// Buzzer ringtones, copied verbatim from UITask::notify() / genericBuzzer so
// the emulated sound matches the device. Mirror these if the firmware changes.
static const char* TUNE_DM      = "MsgRcv3:d=4,o=6,b=200:32e,32g,32b,16c7";
static const char* TUNE_CHAN    = "kerplop:d=16,o=6,b=120:32g#,32c#";
static const char* TUNE_ACK     = "ack:d=32,o=8,b=120:c";
static const char* TUNE_STARTUP = "Startup:d=4,o=5,b=160:16c6,16e6,8g6";

// ---- callback seam so the home screen can reach the controller --------------
struct HomeOwner {
  virtual void openMenu() = 0;
  virtual int  unreadCount() = 0;
};

// Mirror of ChildHomeScreen: status header + big name + unread badge + hint.
class HostChildHome : public UIScreen {
  HomeOwner* _owner;
public:
  explicit HostChildHome(HomeOwner* o) : _owner(o) {}
  int render(DisplayDriver& d) override {
    char timebuf[6];
    time_t now = time(nullptr);
    struct tm* lt = localtime(&now);
    snprintf(timebuf, sizeof(timebuf), "%02d:%02d", lt->tm_hour, lt->tm_min);

    StatusHeader::draw(d, NODE_NAME, timebuf, batteryPercent(FAKE_BATT_MV), true);

    d.setColor(DisplayDriver::LIGHT);
    d.setTextSize(2);
    d.drawTextCentered(d.width() / 2, 26, NODE_NAME);

    int unread = _owner->unreadCount();
    if (unread > 0) {
      char badge[16];
      snprintf(badge, sizeof(badge), "New: %d", unread);
      d.setTextSize(1);
      d.drawTextCentered(d.width() / 2, 42, badge);
    }
    d.setTextSize(1);
    d.drawTextCentered(d.width() / 2, 52, "press = menu");
    return 1000;
  }
  bool handleInput(char c) override {
    if (c == KEY_ENTER || c == KEY_SELECT) { _owner->openMenu(); return true; }
    return false;
  }
};

// Placeholder for the real "full UI" unlocked by the correct PIN (not compiled
// in the thin harness, see README).
class FullUiScreen : public UIScreen {
public:
  int render(DisplayDriver& d) override {
    d.setColor(DisplayDriver::LIGHT);
    d.setTextSize(1);
    d.drawTextCentered(d.width() / 2, 16, "FULL UI UNLOCKED");
    d.drawTextCentered(d.width() / 2, 34, "[sim placeholder]");
    d.drawTextCentered(d.width() / 2, 48, "esc = back");
    return 500;
  }
  bool handleInput(char c) override { return c == KEY_CANCEL || c == KEY_SELECT; }
};

// ---- simulator: mirrors ChildMode's Menu/Pin/Reader state machine -----------
class Sim : public HomeOwner, public MenuHandler, public PinHandler, public ReaderHandler {
  HostDisplay&       _disp;
  ChildConfig        _cfg;
  HostChildHome      _home;
  ListMenuScreen     _menu;
  PinEntryScreen     _pin;
  ChildMessageStore  _store;          // real
  ChildMessageScreen _reader;         // real
  FullUiScreen       _full;
  UIScreen*          _curr;
  char               _toast[80];
  unsigned long      _auto_off;     // deadline (hostMillis) to blank the display
  int                _auto_off_ms;  // AUTO_OFF_MILLIS (15000 on device; env-overridable)

  enum MenuContext { MENU_TOP, MENU_MESSAGES } _menu_context;
  enum ReaderReturn { RETURN_HOME, RETURN_LIST } _reader_return;

  static constexpr const char* TOP_ITEMS[3] = { "Messages", "Settings", "Back" };
  static constexpr const char* NO_MSG_ITEMS[1] = { "No messages" };

public:
  Sim(HostDisplay& d)
    : _disp(d), _home(this), _reader(this), _curr(&_home),
      _menu_context(MENU_TOP), _reader_return(RETURN_HOME) {
    _toast[0] = 0;
    const char* e = getenv("SIM_AUTO_OFF_MS");   // e.g. SIM_AUTO_OFF_MS=3000 to test faster
    _auto_off_ms = e ? atoi(e) : 15000;          // AUTO_OFF_MILLIS default for the OLED L1
    if (_auto_off_ms < 0) _auto_off_ms = 0;
    _auto_off = hostMillis() + _auto_off_ms;
  }

  // --- display sleep/wake, mirroring UITask auto-off + checkDisplayOn ---
  bool displayOn() const { return _disp.isOn(); }
  void tickAutoOff() {     // called each loop after render (as UITask does)
    if (_auto_off_ms > 0 && _disp.isOn() && hostMillis() > _auto_off) _disp.turnOff();
  }
  void noteActivity() { _auto_off = hostMillis() + _auto_off_ms; }
  void onUiKey(int key) {  // checkDisplayOn: if asleep, wake + swallow the press
    if (_auto_off_ms > 0 && !_disp.isOn()) { _disp.turnOn(); noteActivity(); return; }
    noteActivity();
    feedKey(key);
  }
  int secsToSleep() const {
    if (_auto_off_ms == 0 || !_disp.isOn()) return -1;
    unsigned long now = hostMillis();
    return now >= _auto_off ? 0 : (int)((_auto_off - now + 999) / 1000);
  }

  void loadOrSeed() {
    uint8_t buf[CHILD_CONFIG_BLOB_LEN];
    FILE* f = fopen(CFG_PATH, "rb");
    int n = f ? (int)fread(buf, 1, sizeof(buf), f) : 0;
    if (f) fclose(f);
    if (n < CHILD_CONFIG_BLOB_LEN || !childConfigUnpack(_cfg, buf, n)) {
      childConfigInit(_cfg, (uint32_t)CHILD_DEFAULT_PIN);
      save();
    }
  }
  void save() {
    uint8_t buf[CHILD_CONFIG_BLOB_LEN];
    childConfigPack(_cfg, buf);
    FILE* f = fopen(CFG_PATH, "wb");
    if (f) { fwrite(buf, 1, CHILD_CONFIG_BLOB_LEN, f); fclose(f); }
  }

  UIScreen* curr() { return _curr; }
  const char* toast() { return _toast[0] ? _toast : nullptr; }
  void setToast(const char* t) { snprintf(_toast, sizeof(_toast), "%s", t); }
  uint32_t pin() const { return _cfg.pin; }

  // HomeOwner
  void openMenu() override {
    _menu_context = MENU_TOP;
    _menu.set("Menu", TOP_ITEMS, 3, this);
    _curr = &_menu;
  }
  int unreadCount() override { return _store.unread(); }

  void openMessages() {
    _menu_context = MENU_MESSAGES;
    if (_store.count() == 0) _menu.set("Messages", NO_MSG_ITEMS, 1, this);
    else                     _menu.set("Messages", _store.labelPtrs(), _store.count(), this);
    _curr = &_menu;
  }

  // MenuHandler
  void onMenuSelect(int idx) override {
    if (_menu_context == MENU_MESSAGES) {
      if (_store.count() == 0) { openMenu(); return; }
      _reader_return = RETURN_LIST;
      _reader.open(&_store, idx);
      _curr = &_reader;
      return;
    }
    if (idx == 0)      openMessages();
    else if (idx == 1) { _pin.begin("Enter PIN", this); _curr = &_pin; }
    else               _curr = &_home;
  }
  void onMenuCancel() override {
    if (_menu_context == MENU_MESSAGES) _store.markAllRead();
    _curr = &_home;
  }

  // PinHandler
  void onPinEntered(uint32_t entered) override {
    if (entered == _cfg.pin) { hostBuzzerPlay(TUNE_ACK); setToast("PIN ok -> full UI"); _curr = &_full; }
    else                     { setToast("wrong PIN"); _curr = &_home; }
  }
  void onPinCancel() override { _curr = &_home; }

  // ReaderHandler
  void onReaderBack() override {
    if (_reader_return == RETURN_LIST) openMessages();
    else { _store.markAllRead(); _curr = &_home; }
  }

  // Mirror of ChildMode::captureMessage: store it, sound the buzzer (as
  // UITask::notify does), pop the reader on newest.
  void captureMessage(const char* origin, const char* text, bool is_channel) {
    _store.add(origin, text, (uint32_t)time(nullptr), is_channel);
    hostBuzzerPlay(is_channel ? TUNE_CHAN : TUNE_DM);
    _reader_return = RETURN_HOME;
    _reader.open(&_store, 0);
    _curr = &_reader;
    if (_auto_off_ms > 0 && !_disp.isOn()) _disp.turnOn();   // showScreenAwake: light up on rx
    noteActivity();
    char t[64]; snprintf(t, sizeof(t), "rx %s from %s", is_channel ? "chan" : "dm", origin);
    setToast(t);
  }

  // Mirror of ChildMode::onIncomingText for the "!pin old new" command path.
  void injectPinCommand(const char* text) {
    PinChange pc;
    if (parseChildCommand(text, pc) == CHILD_CMD_PIN_CHANGE) {
      if (pc.old_pin == _cfg.pin) { _cfg.pin = pc.new_pin; save(); setToast("pin changed"); }
      else setToast("rx !pin: wrong old pin");
    } else setToast("rx: not a child command");
  }

  void feedKey(int key) { if (_curr) _curr->handleInput((char)key); }
};
constexpr const char* Sim::TOP_ITEMS[3];
constexpr const char* Sim::NO_MSG_ITEMS[1];

int main() {
  HostDisplay disp;
  Sim sim(disp);
  sim.loadOrSeed();

  hostInputBegin();
  fputs("\x1b[2J", stdout);
  hostBuzzerPlay(TUNE_STARTUP);   // device plays this in UITask::begin()

  char status[260];
  bool running = true;
  while (running) {
    int delay = 1000;
    disp.startFrame(DisplayDriver::DARK);
    if (sim.displayOn()) {
      delay = sim.curr()->render(disp);   // blank frame stays dark when asleep
    }

    char sleepinfo[40];
    if (!sim.displayOn()) snprintf(sleepinfo, sizeof(sleepinfo), "[ASLEEP - press a key]");
    else { int s = sim.secsToSleep(); snprintf(sleepinfo, sizeof(sleepinfo), "sleep in %ds", s); }

    snprintf(status, sizeof(status),
      "arrows=move enter=press bksp/esc=back s=select | m=rx dm  n=rx chan  p=!pin  "
      "b=buzzer[%s]  q=quit  | %s  [pin=%u]  %s",
      hostBuzzerMuted() ? "off" : "on", sleepinfo, sim.pin(),
      sim.toast() ? sim.toast() : "");
    disp.setStatusLine(status);
    disp.endFrame();

    sim.tickAutoOff();   // blank the display if the idle deadline passed

    if (delay <= 0 || delay > 500) delay = 500;   // keep the sleep countdown ticking
    int key = hostInputPoll(delay);
    switch (key) {
      case 0:                     break;
      case SIM_KEY_QUIT:          running = false; break;
      case SIM_KEY_INJECT_PIN:    sim.injectPinCommand("!pin 1234 5678"); break;
      case SIM_KEY_INJECT_MSG:    sim.captureMessage("Mom", DEMO_DM, false); break;
      case SIM_KEY_INJECT_CHAN:   sim.captureMessage("#soccer", DEMO_CHAN, true); break;
      case SIM_KEY_TOGGLE_BUZZER: hostBuzzerToggleMute(); break;
      default:                    sim.onUiKey(key); break;   // wake-or-feed
    }
  }

  hostInputEnd();
  fputs("\nbye.\n", stdout);
  return 0;
}
