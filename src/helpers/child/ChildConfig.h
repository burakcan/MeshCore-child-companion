#pragma once
#include <stdint.h>

static const uint8_t CHILD_CONFIG_VERSION = 1;
static const int CHILD_CONFIG_BLOB_LEN = 5;  // 1 version byte + 4 pin bytes (LE)

struct ChildConfig {
  uint8_t  version;
  uint32_t pin;
};

void childConfigInit(ChildConfig& cfg, uint32_t default_pin);
int  childConfigPack(const ChildConfig& cfg, uint8_t* buf);
bool childConfigUnpack(ChildConfig& cfg, const uint8_t* buf, int len);
