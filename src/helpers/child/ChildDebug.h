#pragma once

// opt-in trace for the DM-retry path. compiled out unless CHILD_DEBUG defined (see the
// *_child_debug env) -> expands to nothing, plain child build unaffected. when on, prints
// "CHILD: ..." to USB serial (115200) for live `pio device monitor`.
#if defined(CHILD_DEBUG) && defined(ARDUINO)
  #include <Arduino.h>
  #define CHILD_DEBUG_PRINTLN(F, ...) Serial.printf("CHILD: " F "\n", ##__VA_ARGS__)
#else
  #define CHILD_DEBUG_PRINTLN(...) {}
#endif
