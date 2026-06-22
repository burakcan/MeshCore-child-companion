#include "StatusHeader.h"
#include "UiIcons.h"
#include <stdio.h>

namespace StatusHeader {
void draw(DisplayDriver& d, const char* name, const char* time_str, int batt_pct, bool online,
          int lead_icon) {
  d.setTextSize(1);
  d.setColor(DisplayDriver::LIGHT);

  int title_x = 2;
  if (lead_icon >= 0) {
    int w, h; const uint8_t* b = uiIcon((UiIconId)lead_icon, &w, &h);
    if (b) { d.drawXbm(2, 0, b, w, h); title_x = 2 + w + 2; }
  }

  d.setCursor(title_x, 0);
  char nb[32];   // CP437, translate name first
  d.translateUTF8ToBlocks(nb, name ? name : "", sizeof(nb));
  d.print(nb);

  if (time_str && time_str[0]) {
    d.drawTextCentered(d.width() / 2, 0, time_str);
  }

  // top-right battery, outline + proportional fill. batt_pct<0 skips it (no
  // info, don't draw a misleading empty outline)
  (void)online;
  if (batt_pct >= 0) {
    int bw = 14, bh = 7, bx = d.width() - bw - 4, by = 1;
    d.setColor(DisplayDriver::LIGHT);
    d.drawRect(bx, by, bw, bh);
    d.fillRect(bx + bw, by + 2, 2, bh - 4);        // nub
    int pct = batt_pct > 100 ? 100 : batt_pct;
    int fill = pct * (bw - 2) / 100;
    if (pct > 0 && fill < 1) fill = 1;     // sliver for low-but-alive (1-8%)
    if (fill > 0) d.fillRect(bx + 1, by + 1, fill, bh - 2);
  }

  d.drawRect(0, 10, d.width(), 1);   // separator
}
}
