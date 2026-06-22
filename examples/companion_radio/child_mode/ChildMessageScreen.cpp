#include "ChildMessageScreen.h"
#include <helpers/ui/StatusHeader.h>
#include <helpers/ui/Scrollbar.h>
#include <helpers/ui/UiIcons.h>
#include <string.h>
#include <stdio.h>

void ChildMessageScreen::open(ChildMessageStore* store, int idx) {
  _store = store;
  _idx = idx;
  _scroll = 0;
  _nlines = 0;   // wrap computed in render() against the translated body
}

int ChildMessageScreen::render(DisplayDriver& display) {
  if (!_store) return 1000;
  const ChildMessage& m = _store->at(_idx);

  // CP437 OLED: translate UTF-8 body, then wrap the translated text
  // (so wrap counts and drawn glyphs both line up).
  char body[CHILD_MSG_BODY];
  display.translateUTF8ToBlocks(body, m.text, sizeof(body));
  _nlines = childWrapLines(body, CHILD_MSG_CHARS_PER_LINE, _lines, CHILD_MSG_READER_MAX_LINES);

  char timebuf[6] = "";
  uint32_t mins = (m.timestamp / 60) % 60;
  uint32_t hrs  = (m.timestamp / 3600) % 24;
  snprintf(timebuf, sizeof(timebuf), "%02u:%02u", (unsigned)hrs, (unsigned)mins);
  StatusHeader::draw(display, m.origin, timebuf, -1, true,
                     m.is_channel ? ICON_GROUP_SM : ICON_PERSON_SM);

  display.setColor(DisplayDriver::LIGHT);
  display.setTextSize(1);
  int y = 14;
  char linebuf[CHILD_MSG_CHARS_PER_LINE + 1];
  for (int row = 0; row < CHILD_MSG_READER_VIS; row++) {
    int l = _scroll + row;
    if (l >= _nlines) break;
    int len = _lines[l].len;
    if (len > CHILD_MSG_CHARS_PER_LINE) len = CHILD_MSG_CHARS_PER_LINE;
    memcpy(linebuf, body + _lines[l].start, len);
    linebuf[len] = 0;
    display.setCursor(0, y);
    display.print(linebuf);
    y += 10;
  }
  Scrollbar::draw(display, display.width() - 3, 14, CHILD_MSG_READER_VIS * 10,
                  _nlines, CHILD_MSG_READER_VIS, _scroll);
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
