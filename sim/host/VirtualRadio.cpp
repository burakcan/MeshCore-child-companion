#include "VirtualRadio.h"
#include <Arduino.h>
#include <Packet.h>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctime>

static const char* payloadTypeName(uint8_t t) {
  switch (t) {
    case PAYLOAD_TYPE_REQ:       return "REQ";
    case PAYLOAD_TYPE_RESPONSE:  return "RESPONSE";
    case PAYLOAD_TYPE_TXT_MSG:   return "TXT_MSG";
    case PAYLOAD_TYPE_ACK:       return "ACK";
    case PAYLOAD_TYPE_ADVERT:    return "ADVERT";
    case PAYLOAD_TYPE_GRP_TXT:   return "GRP_TXT";
    case PAYLOAD_TYPE_GRP_DATA:  return "GRP_DATA";
    case PAYLOAD_TYPE_ANON_REQ:  return "ANON_REQ";
    case PAYLOAD_TYPE_PATH:      return "PATH";
    case PAYLOAD_TYPE_TRACE:     return "TRACE";
    case PAYLOAD_TYPE_MULTIPART: return "MULTIPART";
    case PAYLOAD_TYPE_CONTROL:   return "CONTROL";
    case PAYLOAD_TYPE_RAW_CUSTOM:return "RAW_CUSTOM";
    default:                     return "?";
  }
}
static const char* routeTypeName(uint8_t r) {
  switch (r) {
    case ROUTE_TYPE_TRANSPORT_FLOOD:  return "TRANSPORT_FLOOD";
    case ROUTE_TYPE_FLOOD:            return "FLOOD";
    case ROUTE_TYPE_DIRECT:           return "DIRECT";
    case ROUTE_TYPE_TRANSPORT_DIRECT: return "TRANSPORT_DIRECT";
    default:                          return "?";
  }
}

void VirtualRadio::logTx(const uint8_t* bytes, int len) {
  uint8_t header = len > 0 ? bytes[0] : 0;
  uint8_t route = header & PH_ROUTE_MASK;
  uint8_t ptype = (header >> 2) & PH_TYPE_MASK;   // PH_TYPE_SHIFT == 2

  printf("\n\033[1;36m>>> TX\033[0m  %d bytes  route=%s  type=%s(0x%x)\n",
         len, routeTypeName(route), payloadTypeName(ptype), ptype);
  printf("    raw: ");
  for (int i = 0; i < len; i++) {
    printf("%02x", bytes[i]);
    if ((i & 1) == 1) printf(" ");
  }
  printf("\n");
  fflush(stdout);
}

bool VirtualRadio::startSendRaw(const uint8_t* bytes, int len) {
  _tx.assign(bytes, bytes + len);
  _outbox.push_back(_tx);             // for in-process bridging
  if (_udp_fd >= 0) udpSend(bytes, len);   // and out to other processes
  _tx_count++;
  if (_log_tx) logTx(bytes, len);
  _sending = true;
  _send_done_at = millis() + getEstAirtimeFor(len);
  return true;
}

bool VirtualRadio::isSendComplete() {
  return millis() >= _send_done_at;   // simulate airtime, then report done
}

// ---- UDP multicast ether (cross-process mesh) -------------------------------
bool VirtualRadio::enableUdpEther(const char* group, int port) {
  _udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (_udp_fd < 0) return false;
  int yes = 1;
  setsockopt(_udp_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#ifdef SO_REUSEPORT
  setsockopt(_udp_fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes));
#endif
  sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_addr.s_addr = INADDR_ANY; addr.sin_port = htons(port);
  if (bind(_udp_fd, (sockaddr*)&addr, sizeof(addr)) < 0) { close(_udp_fd); _udp_fd = -1; return false; }
  ip_mreq mreq{}; mreq.imr_multiaddr.s_addr = inet_addr(group); mreq.imr_interface.s_addr = INADDR_ANY;
  setsockopt(_udp_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
  unsigned char loop = 1;
  setsockopt(_udp_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
  fcntl(_udp_fd, F_SETFL, O_NONBLOCK);
  _udp_dst.sin_family = AF_INET; _udp_dst.sin_addr.s_addr = inet_addr(group); _udp_dst.sin_port = htons(port);
  _udp_tag = (uint32_t)getpid() * 2654435761u ^ (uint32_t)time(nullptr);   // unique per process
  return true;
}

void VirtualRadio::udpSend(const uint8_t* bytes, int len) {
  uint8_t buf[600];
  if (len > (int)sizeof(buf) - 4) return;
  memcpy(buf, &_udp_tag, 4);
  memcpy(buf + 4, bytes, len);
  sendto(_udp_fd, buf, len + 4, 0, (sockaddr*)&_udp_dst, sizeof(_udp_dst));
}

void VirtualRadio::udpPoll() {
  if (_udp_fd < 0) return;
  uint8_t buf[600];
  for (;;) {
    ssize_t n = recv(_udp_fd, buf, sizeof(buf), 0);
    if (n < 4) break;
    uint32_t tag; memcpy(&tag, buf, 4);
    if (tag == _udp_tag) continue;          // ignore our own transmissions
    inject(buf + 4, (int)n - 4);
  }
}

int VirtualRadio::recvRaw(uint8_t* bytes, int sz) {
  udpPoll();                                 // pull any cross-process frames first
  if (_rx.empty()) return 0;
  Frame f = _rx.front();
  _rx.pop_front();
  int n = (int)f.data.size();
  if (n > sz) n = sz;
  memcpy(bytes, f.data.data(), n);
  _last_snr = f.snr;
  _last_rssi = f.rssi;
  _rx_count++;
  return n;
}
