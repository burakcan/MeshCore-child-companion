#pragma once
#include "DisplayDriver.h"

namespace StatusHeader {
  // lead_icon: UiIconId (>=0) at left, -1 = none
  void draw(DisplayDriver& d, const char* name, const char* time_str, int batt_pct, bool online,
            int lead_icon = -1);
}
