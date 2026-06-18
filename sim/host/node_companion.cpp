// Phase 2: the REAL companion_radio firmware (MyMesh + DataStore) running
// headless on the host. Boots like main.cpp's setup()/loop() but with host
// globals (board/radio_driver/rtc_clock/sensors from target_host.cpp), then
// sends a self-advert so you can watch the real app transmit via VirtualRadio.
#include <Arduino.h>
#include <target.h>                       // board, radio_driver, rtc_clock, sensors
#include <helpers/ArduinoHelpers.h>       // StdRNG
#include <helpers/SimpleMeshTables.h>
#include <helpers/ArduinoSerialInterface.h>
#include "HostFS.h"
#include "MyMesh.h"
#include <cstdio>

// A serial interface with nothing connected (no companion app this phase).
class NullStream : public Stream {
public:
  size_t write(uint8_t) override { return 1; }
  using Print::write;
};

HostFS           InternalFS;              // the host "flash"
StdRNG           fast_rng;
SimpleMeshTables tables;
DataStore        store(InternalFS, rtc_clock);
MyMesh           the_mesh(radio_driver, fast_rng, rtc_clock, tables, store);

int main() {
  Serial.begin(115200);
  printf("=== companion_radio (host) booting ===\n");

  fast_rng.begin(radio_driver.getRngSeed());
  InternalFS.begin();
  store.begin();
  the_mesh.begin(false);                  // no display this phase

  static NullStream nullStream;
  static ArduinoSerialInterface serial_interface;
  serial_interface.begin(nullStream);
  the_mesh.startInterface(serial_interface);
  sensors.begin();

  NodePrefs* prefs = the_mesh.getNodePrefs();
  printf("=== booted. node_name='%s' ===\n", prefs ? prefs->node_name : "?");

  // Trigger a real transmission: a self-advert.
  mesh::Packet* advert = the_mesh.createSelfAdvert(prefs ? prefs->node_name : "HostCompanion");
  if (advert) { the_mesh.sendFlood(advert); printf("queued self-advert\n"); }

  for (int i = 0; i < 400; i++) {         // pump setup()'s loop()
    the_mesh.loop();
    sensors.loop();
    rtc_clock.tick();
    delay(5);
    if (radio_driver.getPacketsSent() > 0 && i > 30) break;
  }
  printf("\n=== done. packets transmitted: %u ===\n", radio_driver.getPacketsSent());
  return 0;
}
