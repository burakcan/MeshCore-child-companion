#pragma once
#include <stdint.h>

inline int batteryPercent(uint16_t millivolts) {
  const int lo = 3300, hi = 4200;
  if (millivolts >= hi) return 100;
  if (millivolts <= lo) return 0;
  return (int)((millivolts - lo) * 100L / (hi - lo));
}
