#pragma once
#include <stdint.h>

// fixed RAM table of pending child-sent DMs awaiting a delivery ACK. pure logic, no
// hardware/heap deps (host-tested). MyMesh owns one and drives the (re)sends; this just
// holds state + computes the schedule.

#define CHILD_OUTBOX_SLOTS            4
#define CHILD_OUTBOX_TEXT_MAX         168       // holds a full DM (<=160) + null
#define CHILD_OUTBOX_BACKOFF_CAP_MS   180000UL  // ~3 min lazy plateau (nudge carries responsiveness)
#define CHILD_OUTBOX_FLOOD_AFTER      2         // consecutive direct misses before a flood retry
#define CHILD_OUTBOX_NUDGE_MIN_GAP_MS 3000UL    // min ms since last send before a heard-packet nudge resends

struct ChildOutboxEntry {
  bool     in_use;
  uint8_t  dest_prefix[6];
  char     text[CHILD_OUTBOX_TEXT_MAX];
  uint32_t timestamp;      // original msg timestamp, held CONSTANT across retries
  uint8_t  attempt;        // last attempt number actually sent
  uint32_t expected_ack;   // 4-byte ack hash currently awaited (changes each attempt)
  uint32_t base_timeout;   // first est_timeout; backoff base
  uint32_t next_retry_at;  // millis() deadline for the next timed retry
  uint32_t last_send_at;   // millis() of the last (re)send; rate-limits the nudge
  uint8_t  direct_misses;  // consecutive unacked sends on a known path -> flood trigger
};

class ChildOutbox {
  ChildOutboxEntry _slots[CHILD_OUTBOX_SLOTS];
  int findByAck(const uint8_t* ack4) const;
  int findByPrefix(const uint8_t dest_prefix[6]) const;
  int freeOrOldestSlot() const;
public:
  ChildOutbox();

  // record a freshly-sent DM. evicts oldest in-use slot if full.
  void enqueue(const uint8_t dest_prefix[6], const char* text, uint32_t timestamp,
               uint8_t attempt, uint32_t expected_ack, uint32_t base_timeout, uint32_t now);

  // free slot matching the 4 ack bytes; true if matched
  bool onAck(const uint8_t* ack4);

  void drop(int slot);   // e.g. recipient contact gone

  int pendingCount() const;
  const ChildOutboxEntry& at(int slot) const { return _slots[slot]; }

  // fill out_slots[CHILD_OUTBOX_SLOTS] with due indices; returns count
  int collectDue(uint32_t now, int out_slots[]);            // timed retry
  int collectNudge(uint32_t now, int out_slots[]);          // heard-packet nudge: gap-limited

  // heard the recipient -> make its entry due now; true if matched
  bool onRecipientHeard(const uint8_t dest_prefix[6], uint32_t now);

  void onResent(int slot, uint8_t new_attempt, uint32_t new_expected_ack, uint32_t now, bool did_flood);

  // push deadline out, keep attempt (resend failed to queue)
  void reschedule(int slot, uint32_t now);

  static uint32_t nextBackoff(uint8_t attempt, uint32_t base_timeout);
  static bool shouldFlood(uint8_t direct_misses) { return direct_misses >= CHILD_OUTBOX_FLOOD_AFTER; }
};
