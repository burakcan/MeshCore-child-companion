// A self-contained "peer" node (a second real mesh node) that can be dropped
// into any sim main to inject network flows at the device: advertise (so the
// device gains a contact) and send DMs (inbound messages). Bridged to the
// device's radio in-process. Used by the control panel and the UI sim.
#pragma once

#include <helpers/ArduinoHelpers.h>
#include <helpers/SimpleMeshTables.h>
#include <helpers/BaseChatMesh.h>
#include <helpers/StaticPoolPacketManager.h>
#include "VirtualRadio.h"
#include <cstdio>
#include <cstring>
#include <vector>

class PanelPeer : public BaseChatMesh {
  ContactInfo* _device = nullptr;
  uint32_t     _ack = 0;
public:
  PanelPeer(mesh::Radio& radio, mesh::RNG& rng, mesh::RTCClock& rtc, SimpleMeshTables& t)
    : BaseChatMesh(radio, *new ArduinoMillis(), rng, rtc, *new StaticPoolPacketManager(16), t) {}

  bool hasDevice() const { return _device != nullptr; }
  const char* deviceName() const { return _device ? _device->name : "(none)"; }

  void advertise() {
    auto* pkt = createSelfAdvert("PanelPeer");
    if (pkt) { sendFlood(pkt); printf("[net] peer advert broadcast\n"); }
  }
  void sendText(const char* text) {
    if (!_device) { printf("[net] no device contact yet (advertise first)\n"); return; }
    uint32_t est = 0;
    sendMessage(*_device, getRTCClock()->getCurrentTime(), 0, text, _ack, est);
    printf("[net] sent to %s: \"%s\"\n", _device->name, text);
  }

  void onDiscoveredContact(ContactInfo& c, bool is_new, uint8_t, const uint8_t*) override {
    _device = &c; printf("[net] peer discovered device: %s (%s)\n", c.name, is_new ? "new" : "known");
  }
  ContactInfo* processAck(const uint8_t* data) override {
    if (_ack && memcmp(data, &_ack, 4) == 0) { printf("[net] device ACKed the message\n"); _ack = 0; }
    return nullptr;
  }
  void onMessageRecv(const ContactInfo& from, mesh::Packet*, uint32_t, const char* text) override {
    printf("[net] device replied (%s): \"%s\"\n", from.name, text);
  }
  void onContactPathUpdated(const ContactInfo&) override {}
  void onCommandDataRecv(const ContactInfo&, mesh::Packet*, uint32_t, const char*) override {}
  void onSignedMessageRecv(const ContactInfo&, mesh::Packet*, uint32_t, const uint8_t*, const char*) override {}
  void onChannelMessageRecv(const mesh::GroupChannel&, mesh::Packet*, uint32_t, const char*) override {}
  uint8_t onContactRequest(const ContactInfo&, uint32_t, const uint8_t*, uint8_t, uint8_t*) override { return 0; }
  void onContactResponse(const ContactInfo&, const uint8_t*, uint8_t) override {}
  uint32_t calcFloodTimeoutMillisFor(uint32_t airtime) const override { return 8000 + airtime; }
  uint32_t calcDirectTimeoutMillisFor(uint32_t airtime, uint8_t) const override { return 4000 + airtime; }
  void onSendTimeout() override { printf("[net] (no ACK)\n"); }
};

// Bundles the peer node + its radio/clocks and the in-process radio bridge.
struct Peer {
  VirtualRadio    radio;
  StdRNG          rng;
  VolatileRTCClock rtc;
  SimpleMeshTables tables;
  PanelPeer       mesh;

  Peer() : mesh(radio, rng, rtc, tables) {}

  void begin(long seed = 0xC0FFEE) {
    rng.begin(seed);
    mesh.self_id = mesh::LocalIdentity(&rng);
    mesh.begin();
    radio.setLogTx(false);     // device TX stays the only thing in the >>> TX monitor
  }
  void loop() { mesh.loop(); }

  // move frames each way between the device radio and the peer radio
  void bridge(VirtualRadio& device) {
    std::vector<uint8_t> f;
    while (device.popOutbound(f)) radio.inject(f.data(), (int)f.size());
    while (radio.popOutbound(f))  device.inject(f.data(), (int)f.size());
  }

  void advertise() { mesh.advertise(); }
  void sendText(const char* t) { mesh.sendText(t); }
};
