#pragma once
#include <stdint.h>

static const uint8_t CHILD_CONFIG_VERSION = 3;
static const int CHILD_CONFIG_BLOB_LEN = 8;  // 1 ver + 4 pin (LE) + 2 tz (LE,signed) + 1 retry_enabled

struct ChildConfig {
  uint8_t  version;
  uint32_t pin;
  int16_t  tz_offset_min;   // minutes added to UTC for the local clock (e.g. Berlin = +60/+120)
  bool     retry_enabled;   // on-device DM retry toggle (OTA "!retry on|off"); default true
};

void childConfigInit(ChildConfig& cfg, uint32_t default_pin);
int  childConfigPack(const ChildConfig& cfg, uint8_t* buf);
bool childConfigUnpack(ChildConfig& cfg, const uint8_t* buf, int len);
