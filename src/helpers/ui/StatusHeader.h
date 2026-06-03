#pragma once
#include "DisplayDriver.h"

namespace StatusHeader {
  void draw(DisplayDriver& d, const char* name, const char* time_str, int batt_pct, bool online);
}
