#include "ChildAlertScreen.h"
#include <helpers/ui/StatusHeader.h>
#include <helpers/ui/UiIcons.h>
#include <helpers/ui/UiFooter.h>
#include <Arduino.h>   // millis()
#include <stdio.h>

void ChildAlertScreen::open(ChildMessageStore* store) {
  _store = store;
  _opened = millis();
}

int ChildAlertScreen::render(DisplayDriver& display) {
  if (!_store || _store->count() == 0) return 1000;
  int nu = _store->unread();
  const ChildMessage& m = _store->at(0);            // newest
  bool is_q = m.is_question;
  int focal = is_q ? ICON_QUESTION : ICON_ENVELOPE;

  // no title bar; the focal icon (envelope vs ?) is the message type. big icon + sender only.
  // blink the icon on arrival to catch the eye.
  uint32_t since = millis() - _opened;
  bool icon_on = (since >= 600) || (((since / 150) % 2) == 0);
  int iw, ih; const uint8_t* ib = uiIcon((UiIconId)focal, &iw, &ih);
  if (ib && icon_on) display.drawXbm(display.width() / 2 - iw / 2, 4, ib, iw, ih);

  display.setColor(DisplayDriver::LIGHT);
  display.setTextSize(2);
  char ob[CHILD_MSG_ORIGIN];   // CP437 OLED: translate UTF-8 origin first
  display.translateUTF8ToBlocks(ob, m.origin, sizeof(ob));
  display.drawTextCentered(display.width() / 2, 24, ob);

  display.setTextSize(1);
  if (nu > 1) {
    char extra[20];
    snprintf(extra, sizeof(extra), "+%d more", nu - 1);
    display.drawTextCentered(display.width() / 2, 46, extra);
  }
  UiFooter::hint(display, is_q ? "press to answer" : "press to read");
  return (since < 600) ? 150 : 1000;   // fast redraw while blinking
}

bool ChildAlertScreen::handleInput(char c) {
  switch ((unsigned char)c) {
    case KEY_ENTER:
    case KEY_SELECT:
    case KEY_RIGHT:
      _owner->onAlertOpen();
      return true;
    case KEY_CANCEL:
    case KEY_LEFT:
      _owner->onAlertDismiss();
      return true;
  }
  return false;
}
