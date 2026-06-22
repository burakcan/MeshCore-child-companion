#pragma once
#include <stdint.h>

static const uint8_t CHILD_CONFIG_VERSION = 2;
static const int CHILD_CONFIG_BLOB_LEN = 7;  // 1 ver + 4 pin (LE) + 2 tz_offset_min (LE, signed)

struct ChildConfig {
  uint8_t  version;
  uint32_t pin;
  int16_t  tz_offset_min;   // minutes added to UTC for the local clock (e.g. Berlin = +60/+120)
};

void childConfigInit(ChildConfig& cfg, uint32_t default_pin);
int  childConfigPack(const ChildConfig& cfg, uint8_t* buf);
bool childConfigUnpack(ChildConfig& cfg, const uint8_t* buf, int len);
