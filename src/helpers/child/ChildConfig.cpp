#include "ChildConfig.h"

void childConfigInit(ChildConfig& cfg, uint32_t default_pin) {
  cfg.version = CHILD_CONFIG_VERSION;
  cfg.pin = default_pin;
}

int childConfigPack(const ChildConfig& cfg, uint8_t* buf) {
  buf[0] = cfg.version;
  buf[1] = (uint8_t)(cfg.pin & 0xFF);
  buf[2] = (uint8_t)((cfg.pin >> 8) & 0xFF);
  buf[3] = (uint8_t)((cfg.pin >> 16) & 0xFF);
  buf[4] = (uint8_t)((cfg.pin >> 24) & 0xFF);
  return CHILD_CONFIG_BLOB_LEN;
}

bool childConfigUnpack(ChildConfig& cfg, const uint8_t* buf, int len) {
  if (len < CHILD_CONFIG_BLOB_LEN) return false;
  if (buf[0] != CHILD_CONFIG_VERSION) return false;
  cfg.version = buf[0];
  cfg.pin = (uint32_t)buf[1] | ((uint32_t)buf[2] << 8)
          | ((uint32_t)buf[3] << 16) | ((uint32_t)buf[4] << 24);
  return true;
}
