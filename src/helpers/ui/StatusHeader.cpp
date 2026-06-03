#include "StatusHeader.h"
#include <stdio.h>

namespace StatusHeader {
void draw(DisplayDriver& d, const char* name, const char* time_str, int batt_pct, bool online) {
  d.setTextSize(1);
  d.setColor(DisplayDriver::LIGHT);
  d.setCursor(2, 0);
  d.print(name);

  if (time_str && time_str[0]) {
    d.drawTextCentered(d.width() / 2, 0, time_str);
  }

  char batt[8];
  snprintf(batt, sizeof(batt), "%d%%", batt_pct);
  d.drawTextRightAlign(d.width() - 2, 0, batt);

  // online indicator dot row separator
  d.drawRect(0, 10, d.width(), 1);
  if (!online) {
    d.setCursor(2, 0);   // overdrawn name already; add an offline marker
    d.drawTextRightAlign(d.width() - 28, 0, "x");
  }
}
}
