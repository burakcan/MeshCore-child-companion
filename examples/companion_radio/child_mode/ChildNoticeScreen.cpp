#include "ChildNoticeScreen.h"
#include <helpers/ui/UiEmptyState.h>
#include <Arduino.h>   // millis()
#include <string.h>

void ChildNoticeScreen::open(int icon, const char* line, uint32_t dismiss_ms) {
  _icon = icon;
  strncpy(_line, line ? line : "", sizeof(_line) - 1);
  _line[sizeof(_line) - 1] = 0;
  _dismiss_ms = dismiss_ms;
  _opened = millis();
}

int ChildNoticeScreen::render(DisplayDriver& display) {
  if (_dismiss_ms > 0 && millis() - _opened >= _dismiss_ms) {
    _owner->onNoticeDone();   // switches screen; hand off so it paints next loop
    return 1;
  }
  UiEmptyState::draw(display, _icon, _line);
  return _dismiss_ms > 0 ? 100 : 1000;   // poll faster while counting down
}

bool ChildNoticeScreen::handleInput(char c) {
  (void)c;
  _owner->onNoticeDone();   // any key dismisses
  return true;
}
