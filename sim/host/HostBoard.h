// Host stand-in for mesh::MainBoard. Only the 4 pure-virtuals need bodies;
// everything else uses MainBoard's defaults. Part of sim/host/, not firmware.
#pragma once
#include <MeshCore.h>

class HostBoard : public mesh::MainBoard {
public:
  void begin() { }   // some apps call board.begin() directly
  uint16_t getBattMilliVolts() override { return 4100; }
  const char* getManufacturerName() const override { return "Host Sim"; }
  void reboot() override { /* no-op on host */ }
  uint8_t getStartupReason() const override { return BD_STARTUP_NORMAL; }
};
