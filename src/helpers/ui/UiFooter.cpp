#include "UiFooter.h"
#include "UiIcons.h"

namespace UiFooter {
void hint(DisplayDriver& d, const char* text, int lead_icon, int y) {
  d.setTextSize(1);
  d.setColor(DisplayDriver::LIGHT);
  const char* t = text ? text : "";
  int w = 0, h = 0;
  const uint8_t* b = (lead_icon >= 0) ? uiIcon((UiIconId)lead_icon, &w, &h) : 0;
  int total = (int)d.getTextWidth(t) + (b ? w + 3 : 0);
  int x = d.width() / 2 - total / 2;            // center icon+text together
  if (x < 0) x = 0;
  if (b) { d.drawXbm(x, y, b, w, h); x += w + 3; }
  d.setCursor(x, y);
  d.print(t);
}
}
