#include "ChildOutbox.h"
#include <string.h>

ChildOutbox::ChildOutbox() {
  memset(_slots, 0, sizeof(_slots));
}

int ChildOutbox::findByAck(const uint8_t* ack4) const {
  uint32_t want;
  memcpy(&want, ack4, 4);
  for (int i = 0; i < CHILD_OUTBOX_SLOTS; i++) {
    if (_slots[i].in_use && _slots[i].expected_ack == want) return i;
  }
  return -1;
}

int ChildOutbox::findByPrefix(const uint8_t dest_prefix[6]) const {
  for (int i = 0; i < CHILD_OUTBOX_SLOTS; i++) {
    if (_slots[i].in_use && memcmp(_slots[i].dest_prefix, dest_prefix, 6) == 0) return i;
  }
  return -1;
}

int ChildOutbox::freeOrOldestSlot() const {
  int oldest = 0;
  for (int i = 0; i < CHILD_OUTBOX_SLOTS; i++) {
    if (!_slots[i].in_use) return i;
    if (_slots[i].timestamp < _slots[oldest].timestamp) oldest = i;
  }
  return oldest;   // table full -> evict oldest by original timestamp
}

void ChildOutbox::enqueue(const uint8_t dest_prefix[6], const char* text, uint32_t timestamp,
                          uint8_t attempt, uint32_t expected_ack, uint32_t base_timeout, uint32_t now) {
  int s = freeOrOldestSlot();
  ChildOutboxEntry& e = _slots[s];
  e.in_use = true;
  memcpy(e.dest_prefix, dest_prefix, 6);
  strncpy(e.text, text, CHILD_OUTBOX_TEXT_MAX - 1);
  e.text[CHILD_OUTBOX_TEXT_MAX - 1] = 0;
  e.timestamp = timestamp;
  e.attempt = attempt;
  e.expected_ack = expected_ack;
  e.base_timeout = base_timeout;
  e.last_send_at = now;
  e.direct_misses = 0;
  e.next_retry_at = now + nextBackoff(attempt, base_timeout);
}

bool ChildOutbox::onAck(const uint8_t* ack4) {
  int s = findByAck(ack4);
  if (s < 0) return false;
  _slots[s].in_use = false;
  return true;
}

void ChildOutbox::drop(int slot) {
  if (slot >= 0 && slot < CHILD_OUTBOX_SLOTS) _slots[slot].in_use = false;
}

int ChildOutbox::pendingCount() const {
  int n = 0;
  for (int i = 0; i < CHILD_OUTBOX_SLOTS; i++) if (_slots[i].in_use) n++;
  return n;
}

uint32_t ChildOutbox::nextBackoff(uint8_t attempt, uint32_t base_timeout) {
  uint32_t v = base_timeout;
  for (uint8_t i = 0; i < attempt && v < CHILD_OUTBOX_BACKOFF_CAP_MS; i++) v <<= 1;
  return v > CHILD_OUTBOX_BACKOFF_CAP_MS ? CHILD_OUTBOX_BACKOFF_CAP_MS : v;
}

int ChildOutbox::collectDue(uint32_t now, int out_slots[]) {
  int n = 0;
  for (int i = 0; i < CHILD_OUTBOX_SLOTS; i++) {
    if (_slots[i].in_use && (int32_t)(now - _slots[i].next_retry_at) >= 0) out_slots[n++] = i;
  }
  return n;
}

void ChildOutbox::onResent(int slot, uint8_t new_attempt, uint32_t new_expected_ack,
                           uint32_t now, bool did_flood) {
  ChildOutboxEntry& e = _slots[slot];
  e.attempt = new_attempt;
  e.expected_ack = new_expected_ack;
  e.last_send_at = now;
  e.next_retry_at = now + nextBackoff(new_attempt, e.base_timeout);
  if (did_flood) e.direct_misses = 0; else e.direct_misses++;
}

void ChildOutbox::reschedule(int slot, uint32_t now) {
  ChildOutboxEntry& e = _slots[slot];
  e.next_retry_at = now + nextBackoff(e.attempt, e.base_timeout);
}

int ChildOutbox::collectNudge(uint32_t now, int out_slots[]) {
  int n = 0;
  for (int i = 0; i < CHILD_OUTBOX_SLOTS; i++) {
    if (_slots[i].in_use &&
        (int32_t)(now - _slots[i].last_send_at) >= (int32_t)CHILD_OUTBOX_NUDGE_MIN_GAP_MS) {
      out_slots[n++] = i;
    }
  }
  return n;
}

bool ChildOutbox::onRecipientHeard(const uint8_t dest_prefix[6], uint32_t now) {
  int s = findByPrefix(dest_prefix);
  if (s < 0) return false;
  _slots[s].next_retry_at = now;   // due now; collectDue picks it up
  return true;
}
