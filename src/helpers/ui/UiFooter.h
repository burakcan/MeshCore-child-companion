#pragma once
#include "DisplayDriver.h"

namespace UiFooter {
  // bottom hint line at row y, optional 8x8 lead icon (UiIconId; -1 = none)
  void hint(DisplayDriver& d, const char* text, int lead_icon = -1, int y = 54);
}
