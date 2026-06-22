#include "UiEmptyState.h"
#include "UiIcons.h"

namespace UiEmptyState {
void draw(DisplayDriver& d, int icon, const char* line) {
  d.setColor(DisplayDriver::LIGHT);
  int w, h; const uint8_t* b = uiIcon((UiIconId)icon, &w, &h);
  if (b) d.drawXbm(d.width()/2 - w/2, 18, b, w, h);
  d.setTextSize(1);
  d.drawTextCentered(d.width()/2, 40, line ? line : "");
}
}
