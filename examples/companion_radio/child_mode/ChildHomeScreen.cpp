#include "ChildHomeScreen.h"
#include "ChildMode.h"
#include <helpers/ui/StatusHeader.h>
#include <helpers/ui/UiIcons.h>
#include <helpers/ui/UiFooter.h>
#include <helpers/ui/BatteryUtils.h>
#include <target.h>            // rtc_clock, board globals
#include "../MyMesh.h"          // the_mesh, NodePrefs
#include <stdio.h>

int ChildHomeScreen::render(DisplayDriver& display) {
  NodePrefs* prefs = the_mesh.getNodePrefs();
  const char* name = prefs ? prefs->node_name : "Mesh";

  int pct = batteryPercent(board.getBattMilliVolts());
  // header = small name + battery; big clock in the center.
  // CHILD_MODE seam: send icon in header while a child-sent DM awaits its ACK
  int lead = the_mesh.childPendingCount() > 0 ? ICON_SEND_SM : -1;
  StatusHeader::draw(display, name, "", pct, true, lead);

  // big clock; "--:--" until the parent sets the time (don't show a wrong clock)
  uint32_t now = rtc_clock.getCurrentTime();
  char clock[6];
  if (now > 946684800u) {   // RTC set (>= year 2000)
    long local = (long)now + (long)_owner->tzOffsetMin() * 60;   // OTA-set tz offset
    if (local < 0) local = 0;
    snprintf(clock, sizeof(clock), "%02u:%02u",
             (unsigned)((local / 3600) % 24), (unsigned)((local / 60) % 60));
  } else {
    snprintf(clock, sizeof(clock), "--:--");
  }
  int unread = _owner->unreadCount();
  int clock_y = unread > 0 ? 24 : 30;   // up a touch to make room for the unread line

  display.setColor(DisplayDriver::LIGHT);
  display.setTextSize(2);
  display.drawTextCentered(display.width() / 2, clock_y, clock);

  // unread count under the clock
  if (unread > 0) {
    display.setTextSize(1);
    int iw, ih; const uint8_t* eb = uiIcon(ICON_ENVELOPE_SM, &iw, &ih);
    char cnt[16]; snprintf(cnt, sizeof(cnt), "%d new", unread);
    int tw = display.getTextWidth(cnt);
    int x = display.width() / 2 - (iw + 3 + tw) / 2;
    int line_y = clock_y + 20;
    if (eb) display.drawXbm(x, line_y, eb, iw, ih);
    display.setCursor(x + iw + 3, line_y + 1);
    display.print(cnt);
  }

  return 1000;
}

bool ChildHomeScreen::handleInput(char c) {
  if (c == KEY_ENTER || c == KEY_SELECT || c == KEY_RIGHT) {
    _owner->openMenu();
    return true;
  }
  return false;
}
