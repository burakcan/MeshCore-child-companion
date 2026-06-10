#include "ChildMessageScreen.h"
#include <helpers/ui/StatusHeader.h>
#include <string.h>
#include <stdio.h>

void ChildMessageScreen::open(ChildMessageStore* store, int idx) {
  _store = store;
  _idx = idx;
  _scroll = 0;
  const ChildMessage& m = store->at(idx);
  _nlines = childWrapLines(m.text, CHILD_MSG_CHARS_PER_LINE, _lines, CHILD_MSG_READER_MAX_LINES);
}

int ChildMessageScreen::render(DisplayDriver& display) {
  if (!_store) return 1000;
  const ChildMessage& m = _store->at(_idx);

  char timebuf[6] = "";
  uint32_t mins = (m.timestamp / 60) % 60;
  uint32_t hrs  = (m.timestamp / 3600) % 24;
  snprintf(timebuf, sizeof(timebuf), "%02u:%02u", (unsigned)hrs, (unsigned)mins);
  StatusHeader::draw(display, m.origin, timebuf, 0, true);

  display.setColor(DisplayDriver::LIGHT);
  display.setTextSize(1);
  int y = 14;
  char linebuf[CHILD_MSG_CHARS_PER_LINE + 1];
  for (int row = 0; row < CHILD_MSG_READER_VIS; row++) {
    int l = _scroll + row;
    if (l >= _nlines) break;
    int len = _lines[l].len;
    if (len > CHILD_MSG_CHARS_PER_LINE) len = CHILD_MSG_CHARS_PER_LINE;
    memcpy(linebuf, m.text + _lines[l].start, len);
    linebuf[len] = 0;
    display.setCursor(0, y);
    display.print(linebuf);
    y += 10;
  }
  // page indicator (page <current>/<total>; _scroll advances in page steps)
  if (_nlines > CHILD_MSG_READER_VIS) {
    int page  = _scroll / CHILD_MSG_READER_VIS + 1;
    int pages = (_nlines + CHILD_MSG_READER_VIS - 1) / CHILD_MSG_READER_VIS;
    char ind[8];
    snprintf(ind, sizeof(ind), "%d/%d", page, pages);
    display.drawTextRightAlign(display.width(), 54, ind);
  }
  return 1000;
}

bool ChildMessageScreen::handleInput(char c) {
  switch ((unsigned char)c) {
    case KEY_UP:
      if (_scroll > 0) _scroll -= CHILD_MSG_READER_VIS;   // page up
      return true;
    case KEY_DOWN:
      if (_scroll + CHILD_MSG_READER_VIS < _nlines) _scroll += CHILD_MSG_READER_VIS;  // page down
      return true;
    case KEY_ENTER:
    case KEY_SELECT:
    case KEY_CANCEL:
    case KEY_LEFT:
      _owner->onReaderBack();
      return true;
  }
  return false;
}
