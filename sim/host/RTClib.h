// Minimal <RTClib.h> stub. MyMesh.h includes it unconditionally, but the
// companion app uses 0 RTClib symbols on host (it uses mesh::RTCClock instead).
#pragma once
#include <stdint.h>

class DateTime {
  uint32_t _t;
public:
  DateTime(uint32_t t = 0) : _t(t) {}
  uint32_t unixtime() const { return _t; }
  // crude UTC breakdown (enough for CommonCLI's clock print)
  uint16_t year() const { return 1970 + _t / 31556952u; }
  uint8_t month() const { return (_t / 2629746u) % 12 + 1; }
  uint8_t day() const { return (_t / 86400u) % 30 + 1; }
  uint8_t hour() const { return (_t / 3600u) % 24; }
  uint8_t minute() const { return (_t / 60u) % 60; }
  uint8_t second() const { return _t % 60; }
};

class RTC_Millis {
public:
  bool begin() { return true; }
  void adjust(const DateTime&) {}
  DateTime now() { return DateTime(0); }
};
