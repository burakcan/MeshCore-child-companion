#include "ChildAlertScreen.h"
#include <helpers/ui/StatusHeader.h>
#include <stdio.h>

int ChildAlertScreen::render(DisplayDriver& display) {
  if (!_store || _store->count() == 0) return 1000;
  int nu = _store->unread();
  const ChildMessage& m = _store->at(0);            // newest
  bool is_q = m.is_question;

  StatusHeader::draw(display, is_q ? "Question" : "New message", "", 0, true);

  display.setColor(DisplayDriver::LIGHT);
  display.setTextSize(2);
  display.drawTextCentered(display.width() / 2, 22, m.origin);

  display.setTextSize(1);
  if (nu > 1) {
    char extra[20];
    snprintf(extra, sizeof(extra), "+%d more", nu - 1);
    display.drawTextCentered(display.width() / 2, 44, extra);
  }
  display.drawTextCentered(display.width() / 2, 54, is_q ? "press = answer" : "press = read");
  return 1000;
}

bool ChildAlertScreen::handleInput(char c) {
  switch ((unsigned char)c) {
    case KEY_ENTER:
    case KEY_SELECT:
      _owner->onAlertOpen();
      return true;
    case KEY_CANCEL:
    case KEY_LEFT:
      _owner->onAlertDismiss();
      return true;
  }
  return false;
}
