#include "PinEntryScreen.h"
#include "UiIcons.h"

void PinEntryScreen::resetDigits() {
  for (int i = 0; i < NUM_DIGITS; i++) _digits[i] = 0;
  _pos = 0;
}

void PinEntryScreen::begin(const char* title, PinHandler* h) {
  _title = title; _handler = h;
  resetDigits();
}

int PinEntryScreen::render(DisplayDriver& display) {
  display.setColor(DisplayDriver::LIGHT);
  int iw, ih; const uint8_t* lk = uiIcon(ICON_LOCK, &iw, &ih);
  if (lk) display.drawXbm(2, 1, lk, iw, ih);
  display.setTextSize(1);
  display.setCursor(lk ? iw + 6 : 2, 4);
  display.print(_title);

  char buf[2] = {0, 0};
  for (int i = 0; i < NUM_DIGITS; i++) {
    int x = 30 + i * 18;
    int y = 30;
    if (i == _pos) {
      display.fillRect(x - 2, y - 2, 14, 16);
      display.setColor(DisplayDriver::DARK);
    } else {
      display.setColor(DisplayDriver::LIGHT);
    }
    buf[0] = (i < _pos) ? '*' : ('0' + _digits[i]);  // mask entered digits
    display.setCursor(x, y);
    display.print(buf);
    display.setColor(DisplayDriver::LIGHT);
  }
  return 200;
}

bool PinEntryScreen::handleInput(char c) {
  switch ((unsigned char)c) {
    case KEY_UP:                                  // +1
      _digits[_pos] = (_digits[_pos] + 1) % 10;
      return true;
    case KEY_DOWN:                                // -1
      _digits[_pos] = (_digits[_pos] + 9) % 10;
      return true;
    case KEY_ENTER:
    case KEY_RIGHT:                               // advance / confirm
      _pos++;
      if (_pos >= NUM_DIGITS) {
        uint32_t pin = 0;
        for (int i = 0; i < NUM_DIGITS; i++) pin = pin * 10 + _digits[i];
        PinHandler* h = _handler;
        resetDigits();
        if (h) h->onPinEntered(pin);
      }
      return true;
    case KEY_SELECT:
    case KEY_CANCEL:
    case KEY_LEFT:                                // back
      resetDigits();
      if (_handler) _handler->onPinCancel();
      return true;
  }
  return false;
}
