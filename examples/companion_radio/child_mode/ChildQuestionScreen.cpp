#include "ChildQuestionScreen.h"
#include <string.h>
#include <stdio.h>

int ChildQuestionScreen::visibleRows() const {
  int option_start_y = _nqlines * 10 + 2;
  int v = (64 - option_start_y) / 12;
  return v < 1 ? 1 : v;
}

void ChildQuestionScreen::open(const ChildQuestion* q, int answered_idx) {
  _q = q;
  _answered_idx = answered_idx;
  _nqlines = childWrapLines(q->question, CHILD_Q_CHARS_PER_LINE, _qlines, CHILD_Q_MAX_QLINES);
  _model.reset(q->num_options, visibleRows());
}

int ChildQuestionScreen::render(DisplayDriver& display) {
  if (!_q) return 1000;
  display.setTextSize(1);

  // question (word-wrapped) across the top
  display.setColor(DisplayDriver::LIGHT);
  char linebuf[CHILD_Q_CHARS_PER_LINE + 1];
  int y = 0;
  for (int i = 0; i < _nqlines; i++) {
    int len = _qlines[i].len;
    if (len > CHILD_Q_CHARS_PER_LINE) len = CHILD_Q_CHARS_PER_LINE;
    memcpy(linebuf, _q->question + _qlines[i].start, len);
    linebuf[len] = 0;
    display.setCursor(0, y);
    display.print(linebuf);
    y += 10;
  }

  // options list below
  int option_start_y = _nqlines * 10 + 2;
  int visible = visibleRows();
  int top = _model.top();
  char optbuf[CHILD_Q_OPTLEN + 4];
  for (int row = 0; row < visible; row++) {
    int idx = top + row;
    if (idx >= _q->num_options) break;
    int oy = option_start_y + row * 12;
    if (idx == _model.selected()) {
      display.setColor(DisplayDriver::LIGHT);
      display.fillRect(0, oy - 1, display.width(), 12);
      display.setColor(DisplayDriver::DARK);
    } else {
      display.setColor(DisplayDriver::LIGHT);
    }
    if (idx == _answered_idx) {
      snprintf(optbuf, sizeof(optbuf), "> %s", _q->options[idx]);  // chosen-answer marker (ASCII, CP437-safe)
    } else {
      snprintf(optbuf, sizeof(optbuf), "%s", _q->options[idx]);
    }
    display.setCursor(4, oy);
    display.print(optbuf);
  }
  return 250;
}

bool ChildQuestionScreen::handleInput(char c) {
  switch ((unsigned char)c) {
    case KEY_UP:
      _model.prev();
      return true;
    case KEY_DOWN:
      _model.next();
      return true;
    case KEY_ENTER:
    case KEY_SELECT:
    case KEY_RIGHT:
      if (_answered_idx >= 0) _owner->onQuestionCancel();          // locked -> exit
      else                    _owner->onQuestionSelect(_model.selected());
      return true;
    case KEY_CANCEL:
    case KEY_LEFT:
      _owner->onQuestionCancel();
      return true;
  }
  return false;
}
