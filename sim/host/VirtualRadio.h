// VirtualRadio: a host implementation of mesh::Radio with no real RF.
//   - TX: every packet the firmware transmits passes through startSendRaw();
//         we capture, decode, and log it (this is the "see sent messages" tap).
//   - RX: inject() queues a raw frame that recvRaw() will hand up the stack.
// The single TX chokepoint is Dispatcher::checkSend -> startSendRaw, and the
// single RX chokepoint is Dispatcher::checkRecv -> recvRaw.
#pragma once
#include <Dispatcher.h>   // mesh::Radio
#include <stdint.h>
#include <deque>
#include <vector>
#include <netinet/in.h>   // sockaddr_in for the UDP ether

class VirtualRadio : public mesh::Radio {
  struct Frame { std::vector<uint8_t> data; float snr; int rssi; };
  std::deque<Frame> _rx;       // injected inbound frames
  std::vector<uint8_t> _tx;    // frame currently "in the air"
  std::deque<std::vector<uint8_t>> _outbox;  // transmitted frames, for bridging
  bool _log_tx = true;
  bool _sending = false;
  unsigned long _send_done_at = 0;
  float _last_snr = 8.0f;
  int   _last_rssi = -60;

  void logTx(const uint8_t* bytes, int len);

  int         _udp_fd = -1;
  uint32_t    _udp_tag = 0;
  sockaddr_in _udp_dst{};
  void        udpSend(const uint8_t* bytes, int len);
  void        udpPoll();

public:
  // Inject an inbound raw frame (as if received over the air).
  void inject(const uint8_t* bytes, int len, float snr = 8.0f, int rssi = -60) {
    _rx.push_back({ std::vector<uint8_t>(bytes, bytes + len), snr, rssi });
  }
  size_t txCount() const { return _tx_count; }

  // Bridging two nodes: drain each transmitted frame to deliver to a peer radio.
  void setLogTx(bool on) { _log_tx = on; }
  bool popOutbound(std::vector<uint8_t>& out) {
    if (_outbox.empty()) return false;
    out = _outbox.front(); _outbox.pop_front(); return true;
  }

  // UDP multicast "ether": connect this radio to other sim PROCESSES on the same
  // channel, so e.g. a control-panel process and a UI process form a real mesh.
  bool enableUdpEther(const char* group = "239.71.71.71", int port = 7337);

  // --- mesh::Radio interface ---
  int recvRaw(uint8_t* bytes, int sz) override;
  uint32_t getEstAirtimeFor(int len_bytes) override { return (uint32_t)len_bytes * 8; } // ~plausible ms
  float packetScore(float snr, int packet_len) override { return snr; }
  bool startSendRaw(const uint8_t* bytes, int len) override;
  bool isSendComplete() override;
  void onSendFinished() override { _sending = false; }
  bool isInRecvMode() const override { return !_sending; }

  bool  isReceiving() override { return false; }   // never busy -> TX is always allowed
  float getLastRSSI() const override { return (float)_last_rssi; }
  float getLastSNR() const override { return _last_snr; }

  // RadioLibWrapper-compatible API the firmware calls on radio_driver (no-ops here)
  void setParams(float, float, uint8_t, uint8_t) {}
  void setTxPower(int8_t) {}
  void setRxBoostedGainMode(bool) {}
  uint32_t getPacketsRecv() const { return _rx_count; }
  uint32_t getPacketsRecvErrors() const { return 0; }
  uint32_t getPacketsSent() const { return (uint32_t)_tx_count; }
  void resetStats() { _tx_count = 0; _rx_count = 0; }
  void powerOff() {}
  void standby() {}
  uint32_t getRngSeed() { return 0x6d657368; }   // 'mesh'

private:
  size_t _tx_count = 0;
  size_t _rx_count = 0;
};
