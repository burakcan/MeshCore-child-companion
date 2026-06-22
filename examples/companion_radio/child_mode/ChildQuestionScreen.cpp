#include "ChildQuestionScreen.h"
#include <helpers/ui/UiIcons.h>
#include <helpers/ui/Scrollbar.h>
#include <string.h>
#include <stdio.h>

static const int Q_TOP = 11;   // y of the wrapped question (below the "<from> asks" line)

int ChildQuestionScreen::visibleRows() const {
  int option_start_y = Q_TOP + _nqlines * 10 + 2;
  int v = (64 - option_start_y) / 12;
  return v < 1 ? 1 : v;
}

void ChildQuestionScreen::open(const ChildQuestion* q, int answered_idx, const char* from) {
  _q = q;
  _from = from ? from : "";
  _answered_idx = answered_idx;
  // defer option-model reset to first render(): visible-row count must match the translated
  // wrap, not a raw-UTF-8 byte-count wrap.
  _need_layout = true;
}

int ChildQuestionScreen::render(DisplayDriver& display) {
  if (!_q) return 1000;
  display.setTextSize(1);

  // CP437 OLED: translate then wrap (correct counts + glyphs)
  char qtext[CHILD_Q_TEXTLEN];
  display.translateUTF8ToBlocks(qtext, _q->question, sizeof(qtext));
  _nqlines = childWrapLines(qtext, CHILD_Q_CHARS_PER_LINE, _qlines, CHILD_Q_MAX_QLINES);
  if (_need_layout) { _model.reset(_q->num_options, visibleRows()); _need_layout = false; }

  // who asked, across the top
  display.setColor(DisplayDriver::LIGHT);
  char fb[24];
  display.translateUTF8ToBlocks(fb, _from, sizeof(fb));
  char fromline[40];
  snprintf(fromline, sizeof(fromline), "%s asks:", fb);
  display.setCursor(0, 0);
  display.print(fromline);

  // wrapped question below the sender line
  char linebuf[CHILD_Q_CHARS_PER_LINE + 1];
  int y = Q_TOP;
  for (int i = 0; i < _nqlines; i++) {
    int len = _qlines[i].len;
    if (len > CHILD_Q_CHARS_PER_LINE) len = CHILD_Q_CHARS_PER_LINE;
    memcpy(linebuf, qtext + _qlines[i].start, len);
    linebuf[len] = 0;
    display.setCursor(0, y);
    display.print(linebuf);
    y += 10;
  }

  // options list below
  int option_start_y = Q_TOP + _nqlines * 10 + 2;
  int visible = visibleRows();
  int top = _model.top();
  for (int row = 0; row < visible; row++) {
    int idx = top + row;
    if (idx >= _q->num_options) break;
    int oy = option_start_y + row * 12;
    display.setColor(DisplayDriver::LIGHT);

    // left gutter: caret on focused row, check on the chosen answer (no inverse bar)
    if (idx == _answered_idx) {
      int iw, ih; const uint8_t* cb = uiIcon(ICON_CHECK_SM, &iw, &ih);
      if (cb) display.drawXbm(1, oy + (12 - ih) / 2 - 1, cb, iw, ih);
    } else if (idx == _model.selected()) {
      int cw, ch; const uint8_t* cv = uiIcon(ICON_CHEVRON_RIGHT, &cw, &ch);
      if (cv) display.drawXbm(1, oy + (12 - ch) / 2 - 1, cv, cw, ch);
    }
    char ot[CHILD_Q_OPTLEN];   // CP437: translate option text first
    display.translateUTF8ToBlocks(ot, _q->options[idx], sizeof(ot));
    display.setCursor(10, oy);
    display.print(ot);
  }

  Scrollbar::draw(display, display.width() - 3, option_start_y, visible * 12,
                  _q->num_options, visible, top);
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
