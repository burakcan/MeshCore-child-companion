#include "ListMenuScreen.h"

static const int ROW_H = 12;
static const int HEADER_Y = 0;
static const int FIRST_ROW_Y = 14;
static const int VISIBLE_ROWS = 4;   // (64 - 14) / 12

void ListMenuScreen::set(const char* title, const char* const* items, int count, MenuHandler* h) {
  _title = title; _items = items; _count = count; _handler = h;
  _model.reset(count, VISIBLE_ROWS);
}

int ListMenuScreen::render(DisplayDriver& display) {
  display.setTextSize(1);
  display.setColor(DisplayDriver::LIGHT);
  display.setCursor(2, HEADER_Y);
  display.print(_title);

  int top = _model.top();
  for (int row = 0; row < VISIBLE_ROWS; row++) {
    int idx = top + row;
    if (idx >= _count) break;
    int y = FIRST_ROW_Y + row * ROW_H;
    if (idx == _model.selected()) {
      display.setColor(DisplayDriver::LIGHT);
      display.fillRect(0, y - 1, display.width(), ROW_H);
      display.setColor(DisplayDriver::DARK);
    } else {
      display.setColor(DisplayDriver::LIGHT);
    }
    display.setCursor(4, y);
    display.print(_items[idx]);
  }
  return 250;   // refresh cadence (ms)
}

bool ListMenuScreen::handleInput(char c) {
  unsigned char uc = (unsigned char)c;
  switch (uc) {
    case KEY_UP:    _model.prev(); return true;
    case KEY_DOWN:  _model.next(); return true;
    case KEY_ENTER:
    case KEY_SELECT:
    case KEY_RIGHT:                       // select
      if (_handler && _count > 0) _handler->onMenuSelect(_model.selected());
      return true;
    case KEY_CANCEL:
    case KEY_LEFT:                        // back
      if (_handler) _handler->onMenuCancel();
      return true;
  }
  return false;
}
