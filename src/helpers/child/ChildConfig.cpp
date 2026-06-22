#include "ChildConfig.h"

void childConfigInit(ChildConfig& cfg, uint32_t default_pin) {
  cfg.version = CHILD_CONFIG_VERSION;
  cfg.pin = default_pin;
  cfg.tz_offset_min = 0;
}

int childConfigPack(const ChildConfig& cfg, uint8_t* buf) {
  buf[0] = CHILD_CONFIG_VERSION;
  buf[1] = (uint8_t)(cfg.pin & 0xFF);
  buf[2] = (uint8_t)((cfg.pin >> 8) & 0xFF);
  buf[3] = (uint8_t)((cfg.pin >> 16) & 0xFF);
  buf[4] = (uint8_t)((cfg.pin >> 24) & 0xFF);
  uint16_t tz = (uint16_t)cfg.tz_offset_min;   // two's complement, LE
  buf[5] = (uint8_t)(tz & 0xFF);
  buf[6] = (uint8_t)((tz >> 8) & 0xFF);
  return CHILD_CONFIG_BLOB_LEN;
}

bool childConfigUnpack(ChildConfig& cfg, const uint8_t* buf, int len) {
  if (len < 5) return false;                 // need v1 minimum (version + pin)
  uint8_t ver = buf[0];
  if (ver != 1 && ver != 2) return false;
  cfg.version = CHILD_CONFIG_VERSION;        // upgrade in place; next save() rewrites as v2
  cfg.pin = (uint32_t)buf[1] | ((uint32_t)buf[2] << 8)
          | ((uint32_t)buf[3] << 16) | ((uint32_t)buf[4] << 24);
  if (ver >= 2 && len >= 7) {
    cfg.tz_offset_min = (int16_t)((uint16_t)buf[5] | ((uint16_t)buf[6] << 8));
  } else {
    cfg.tz_offset_min = 0;                   // v1: no tz
  }
  return true;
}
