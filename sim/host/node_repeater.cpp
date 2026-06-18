// simple_repeater firmware running headless on the host. Mirrors
// examples/simple_repeater/main.cpp setup()/loop() with host globals.
// Demonstrates the simulator is role-agnostic: same host shims, different app.
#include <Arduino.h>
#include <target.h>                  // board, radio_driver, rtc_clock, sensors
#include <Mesh.h>
#include <helpers/ArduinoHelpers.h>
#include <helpers/SimpleMeshTables.h>
#include <helpers/IdentityStore.h>
#include "HostFS.h"
#include "MyMesh.h"                  // examples/simple_repeater/MyMesh.h (via -I)
#include <cstdio>

HostFS           InternalFS;
StdRNG           fast_rng;
SimpleMeshTables tables;
MyMesh the_mesh(board, radio_driver, *new ArduinoMillis(), fast_rng, rtc_clock, tables);

int main() {
  Serial.begin(115200);
  printf("=== simple_repeater (host) booting ===\n");
  board.begin();
  fast_rng.begin(radio_driver.getRngSeed());
  InternalFS.begin();

  IdentityStore store(InternalFS, "");
  store.begin();
  if (!store.load("_main", the_mesh.self_id)) {
    the_mesh.self_id = radio_new_identity();
    store.save("_main", the_mesh.self_id);
  }
  the_mesh.begin(&InternalFS);
  sensors.begin();

  printf("=== booted. pubkey[0..3]=%02x%02x%02x%02x ===\n",
         the_mesh.self_id.pub_key[0], the_mesh.self_id.pub_key[1],
         the_mesh.self_id.pub_key[2], the_mesh.self_id.pub_key[3]);

  the_mesh.sendSelfAdvertisement(200, false);   // small delay so it fires promptly

  for (int i = 0; i < 800; i++) {
    the_mesh.loop();
    sensors.loop();
    rtc_clock.tick();
    delay(5);
    if (radio_driver.getPacketsSent() > 0 && i > 60) break;
  }
  printf("\n=== repeater done. packets transmitted: %u ===\n", radio_driver.getPacketsSent());
  return 0;
}
