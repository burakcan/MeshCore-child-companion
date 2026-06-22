#include "ListMenuScreen.h"
#include "Scrollbar.h"
#include "UiIcons.h"

static const int ROW_H = 12;
static const int HEADER_Y = 0;
static const int FIRST_ROW_Y = 14;
static const int VISIBLE_ROWS = 4;   // (64 - 14) / 12

void ListMenuScreen::set(const char* title, const char* const* items, int count, MenuHandler* h,
                         const int* icons) {
  _title = title; _items = items; _count = count; _handler = h; _icons = icons;
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
    display.setColor(DisplayDriver::LIGHT);

    // left caret in a fixed gutter (no inverse bar: keeps scrollbar + row icons visible)
    if (idx == _model.selected()) {
      int cw, ch; const uint8_t* cv = uiIcon(ICON_CHEVRON_RIGHT, &cw, &ch);
      if (cv) display.drawXbm(0, y + (ROW_H - ch) / 2 - 1, cv, cw, ch);
    }
    int text_x = 10;   // past the caret gutter
    if (_icons && _icons[idx] >= 0) {
      int iw, ih; const uint8_t* ib = uiIcon((UiIconId)_icons[idx], &iw, &ih);
      if (ib) { display.drawXbm(text_x, y + (ROW_H - ih) / 2 - 1, ib, iw, ih); text_x += iw + 2; }
    }
    char buf[64];   // CP437: translate UTF-8 to blocks, then ellipsize to one line
    display.translateUTF8ToBlocks(buf, _items[idx], sizeof(buf));
    int avail = display.width() - text_x - (_count > VISIBLE_ROWS ? 5 : 0);
    display.drawTextEllipsized(text_x, y, avail, buf);
  }

  // track+thumb on the right edge when the list overflows
  Scrollbar::draw(display, display.width() - 3, FIRST_ROW_Y, VISIBLE_ROWS * ROW_H,
                  _count, VISIBLE_ROWS, top);
  return 250;   // ms
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
