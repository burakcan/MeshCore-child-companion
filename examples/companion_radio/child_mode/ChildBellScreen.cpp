#include "ChildBellScreen.h"
#include <helpers/ui/StatusHeader.h>
#include <helpers/ui/UiIcons.h>
#include <helpers/ui/UiFooter.h>
#include <Arduino.h>   // millis()
#include <string.h>

void ChildBellScreen::open(const char* caller) {
  strncpy(_caller, caller ? caller : "", sizeof(_caller) - 1);
  _caller[sizeof(_caller) - 1] = 0;
  _last_ring = millis();          // raiseBell does the first ring; wait one interval for the next
}

int ChildBellScreen::render(DisplayDriver& display) {
  StatusHeader::draw(display, "Calling", "", -1, true);

  // bell icon, wobbling side to side
  int iw, ih; const uint8_t* bb = uiIcon(ICON_BELL, &iw, &ih);
  int dx = ((millis() / 200) % 2) ? 2 : -2;
  if (bb) display.drawXbm(display.width() / 2 - iw / 2 + dx, 15, bb, iw, ih);

  display.setColor(DisplayDriver::LIGHT);
  display.setTextSize(2);
  char cb[32];   // CP437 OLED: translate UTF-8 caller name first
  display.translateUTF8ToBlocks(cb, _caller, sizeof(cb));
  display.drawTextCentered(display.width() / 2, 36, cb);

  UiFooter::hint(display, "press = I'm here");
  return 200;   // wobble redraw cadence; ring re-fires independently in poll()
}

void ChildBellScreen::poll() {
  // re-ring on cadence even while display asleep: poll() runs every loop, render() doesn't.
  // onBellRingTick() no-ops once the window elapses.
  if (millis() - _last_ring >= CHILD_BELL_RING_INTERVAL_MS) {
    _last_ring = millis();
    _owner->onBellRingTick();
  }
}

bool ChildBellScreen::handleInput(char c) {
  switch ((unsigned char)c) {
    case KEY_ENTER:
    case KEY_SELECT:
    case KEY_RIGHT:
    case KEY_CANCEL:
    case KEY_LEFT:
      _owner->onBellDismiss();           // one press acks; nothing to open
      return true;
  }
  return false;
}
