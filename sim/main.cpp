// PC simulator for the MeshCore child-mode UI (Wio Tracker L1). Builds the real
// Arduino-free firmware code (widgets, child/ logic, ChildMessageScreen) and
// drives it from a host DisplayDriver (terminal) + keyboard.
//
// This harness compiles the REAL, Arduino-free UI widgets and child-mode logic
// from the firmware tree (ListMenuScreen, PinEntryScreen, StatusHeader,
// MenuModel, ChildConfig, ChildCommands) and drives them with a host
// DisplayDriver (terminal renderer) + keyboard input.
//
// The screen-flow glue below MIRRORS examples/companion_radio/child_mode/
// ChildMode.cpp and ChildHomeScreen.cpp. Those two files are NOT compiled here
// because they pull in MyMesh.h / UITask.h (full firmware). If the UI agent
// changes the flow there, mirror the change here. Everything you actually SEE
// rendered (menu, PIN pad, status header) is the real firmware code.

#include <helpers/ui/ListMenuScreen.h>
#include <helpers/ui/PinEntryScreen.h>
#include <helpers/ui/StatusHeader.h>
#include <helpers/ui/BatteryUtils.h>
#include <helpers/child/ChildConfig.h>
#include <helpers/child/ChildCommands.h>

#include "host_display.h"
#include "host_input.h"

#include <cstdio>
#include <cstring>
#include <ctime>

#ifndef CHILD_DEFAULT_PIN
#define CHILD_DEFAULT_PIN 1234
#endif

static const char* CFG_PATH = "sim/child_cfg.bin";
static const char* NODE_NAME = "Mesh";
static const uint16_t FAKE_BATT_MV = 4100;

// ---- callback seam so the home screen can open the menu ----------------------
struct HomeOwner { virtual void openMenu() = 0; };

// Mirror of ChildHomeScreen: status header + big name + "press = menu".
class HostChildHome : public UIScreen {
  HomeOwner* _owner;
public:
  explicit HostChildHome(HomeOwner* o) : _owner(o) {}
  int render(DisplayDriver& d) override {
    char timebuf[6];
    time_t now = time(nullptr);
    struct tm* lt = localtime(&now);
    snprintf(timebuf, sizeof(timebuf), "%02d:%02d", lt->tm_hour, lt->tm_min);

    int pct = batteryPercent(FAKE_BATT_MV);
    StatusHeader::draw(d, NODE_NAME, timebuf, pct, true);

    d.setColor(DisplayDriver::LIGHT);
    d.setTextSize(2);
    d.drawTextCentered(d.width() / 2, 26, NODE_NAME);
    d.setTextSize(1);
    d.drawTextCentered(d.width() / 2, 52, "press = menu");
    return 1000;
  }
  bool handleInput(char c) override {
    if (c == KEY_ENTER || c == KEY_SELECT) { _owner->openMenu(); return true; }
    return false;
  }
};

// Placeholder for the "full UI" you unlock with the correct PIN.
class FullUiScreen : public UIScreen {
public:
  int render(DisplayDriver& d) override {
    d.setColor(DisplayDriver::LIGHT);
    d.setTextSize(1);
    d.drawTextCentered(d.width() / 2, 20, "FULL UI UNLOCKED");
    d.drawTextCentered(d.width() / 2, 40, "(esc = back)");
    return 500;
  }
  bool handleInput(char c) override { return c == KEY_CANCEL || c == KEY_SELECT; }
};

// ---- the simulator: mirrors ChildMode's MenuHandler/PinHandler flow ----------
class Sim : public HomeOwner, public MenuHandler, public PinHandler {
  HostDisplay& _disp;
  ChildConfig _cfg;
  HostChildHome _home;
  ListMenuScreen _menu;
  PinEntryScreen _pin;
  FullUiScreen _full;
  UIScreen* _curr;
  char _toast[64];

  static constexpr const char* MENU_ITEMS[2] = { "Settings", "Back" };

public:
  Sim(HostDisplay& d) : _disp(d), _home(this), _curr(&_home) { _toast[0] = 0; }

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
    _menu.set("Menu", MENU_ITEMS, 2, this);
    _curr = &_menu;
  }
  // MenuHandler
  void onMenuSelect(int idx) override {
    if (idx == 0) { _pin.begin("Enter PIN", this); _curr = &_pin; }
    else          { _curr = &_home; }
  }
  void onMenuCancel() override { _curr = &_home; }
  // PinHandler
  void onPinEntered(uint32_t entered) override {
    if (entered == _cfg.pin) { setToast("PIN ok -> full UI"); _curr = &_full; }
    else                     { setToast("wrong PIN"); _curr = &_home; }
  }
  void onPinCancel() override { _curr = &_home; }

  // Mirror of ChildMode::onIncomingText for the "!pin old new" command.
  void injectIncoming(const char* text) {
    PinChange pc;
    if (parseChildCommand(text, pc) == CHILD_CMD_PIN_CHANGE) {
      if (pc.old_pin == _cfg.pin) {
        _cfg.pin = pc.new_pin; save();
        char t[64]; snprintf(t, sizeof(t), "rx '%s' -> pin now %u", text, _cfg.pin);
        setToast(t);
      } else {
        setToast("rx !pin: wrong old pin");
      }
    } else {
      setToast("rx: not a child command");
    }
  }

  void feedKey(int key) { if (_curr) _curr->handleInput((char)key); }
};
constexpr const char* Sim::MENU_ITEMS[2];

int main() {
  HostDisplay disp;
  Sim sim(disp);
  sim.loadOrSeed();

  hostInputBegin();
  fputs("\x1b[2J", stdout);  // clear screen once

  char status[160];
  bool running = true;
  while (running) {
    disp.startFrame(DisplayDriver::DARK);
    int delay = sim.curr()->render(disp);

    snprintf(status, sizeof(status),
      "arrows=joystick  enter=press  bksp=back  s=select  p=inject !pin  q=quit"
      "   [pin=%u]  %s", sim.pin(), sim.toast() ? sim.toast() : "");
    disp.setStatusLine(status);
    disp.endFrame();

    int key = hostInputPoll(delay > 0 ? delay : 100);
    if (key == 0) continue;
    if (key == SIM_KEY_QUIT) { running = false; }
    else if (key == SIM_KEY_INJECT_PIN) { sim.injectIncoming("!pin 1234 5678"); }
    else { sim.feedKey(key); }
  }

  hostInputEnd();
  fputs("\nbye.\n", stdout);
  return 0;
}
