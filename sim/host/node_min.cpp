// Phase 1 proof: a minimal real MeshCore node running on the host. It builds a
// real Identity, creates a self-advert, and sends it through the real
// Dispatcher/Mesh -> VirtualRadio, which decodes and logs the transmitted
// packet. No companion protocol, no DataStore yet (that's Phase 2).
#include <Arduino.h>
#include <Mesh.h>
#include <helpers/ArduinoHelpers.h>
#include <helpers/SimpleMeshTables.h>
#include <helpers/StaticPoolPacketManager.h>
#include "HostBoard.h"
#include "VirtualRadio.h"
#include <cstdio>

// Minimal concrete Mesh, no app behavior, just enough to send.
class MinNode : public mesh::Mesh {
public:
  MinNode(mesh::Radio& r, mesh::MillisecondClock& ms, mesh::RNG& rng,
          mesh::RTCClock& rtc, mesh::PacketManager& mgr, mesh::MeshTables& t)
    : mesh::Mesh(r, ms, rng, rtc, mgr, t) {}
};

int main() {
  HostBoard       board;
  VirtualRadio    radio;
  ArduinoMillis   ms;
  StdRNG          rng;
  VolatileRTCClock rtc;
  StaticPoolPacketManager mgr(16);
  SimpleMeshTables tables;

  rng.begin(0x1234);
  MinNode node(radio, ms, rng, rtc, mgr, tables);
  node.self_id = mesh::LocalIdentity(&rng);   // real Ed25519 keypair
  node.begin();

  printf("=== MeshCore host node up. self pubkey[0..3] = %02x%02x%02x%02x ===\n",
         node.self_id.pub_key[0], node.self_id.pub_key[1],
         node.self_id.pub_key[2], node.self_id.pub_key[3]);

  const char* name = "HostSimNode";
  mesh::Packet* advert = node.createAdvert(node.self_id, (const uint8_t*)name, strlen(name));
  if (!advert) { printf("createAdvert failed\n"); return 1; }
  node.sendFlood(advert);
  printf("queued self-advert; pumping dispatcher...\n");

  for (int i = 0; i < 200; i++) {   // pump until the send drains through the radio
    node.loop();
    delay(5);
    if (radio.txCount() > 0 && i > 20) break;
  }

  printf("\n=== done. packets transmitted: %zu ===\n", radio.txCount());
  return 0;
}
