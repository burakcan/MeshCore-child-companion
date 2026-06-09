#include "ChildHomeScreen.h"
#include "ChildMode.h"
#include <helpers/ui/StatusHeader.h>
#include <helpers/ui/BatteryUtils.h>
#include <target.h>            // rtc_clock, board globals
#include "../MyMesh.h"          // the_mesh, NodePrefs
#include <stdio.h>

int ChildHomeScreen::render(DisplayDriver& display) {
  NodePrefs* prefs = the_mesh.getNodePrefs();
  const char* name = prefs ? prefs->node_name : "Mesh";

  char timebuf[6] = "";
  uint32_t now = rtc_clock.getCurrentTime();
  uint32_t mins = (now / 60) % 60;
  uint32_t hrs  = (now / 3600) % 24;
  snprintf(timebuf, sizeof(timebuf), "%02u:%02u", (unsigned)hrs, (unsigned)mins);

  int pct = batteryPercent(board.getBattMilliVolts());
  StatusHeader::draw(display, name, timebuf, pct, true);

  display.setColor(DisplayDriver::LIGHT);
  display.setTextSize(2);
  display.drawTextCentered(display.width() / 2, 26, name);

  int unread = _owner->unreadCount();
  if (unread > 0) {
    char badge[16];
    snprintf(badge, sizeof(badge), "New: %d", unread);
    display.setColor(DisplayDriver::LIGHT);
    display.setTextSize(1);
    display.drawTextCentered(display.width() / 2, 42, badge);
  }

  display.setTextSize(1);
  display.drawTextCentered(display.width() / 2, 52, "press = menu");
  return 1000;
}

bool ChildHomeScreen::handleInput(char c) {
  if (c == KEY_ENTER || c == KEY_SELECT) {
    _owner->openMenu();
    return true;
  }
  return false;
}
