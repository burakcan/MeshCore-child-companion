#include <gtest/gtest.h>
#include <string.h>
#include <helpers/child/ChildOutbox.h>

static void prefix(uint8_t out[6], uint8_t seed) {
  for (int i = 0; i < 6; i++) out[i] = seed + i;
}

TEST(ChildOutbox, EnqueueTracksPending) {
  ChildOutbox box;
  EXPECT_EQ(box.pendingCount(), 0);
  uint8_t p[6]; prefix(p, 10);
  box.enqueue(p, "hi", 1000, 0, 0xAABBCCDD, 2000, 5000);
  EXPECT_EQ(box.pendingCount(), 1);
  const ChildOutboxEntry& e = box.at(0);
  EXPECT_TRUE(e.in_use);
  EXPECT_EQ(e.timestamp, 1000u);
  EXPECT_STREQ(e.text, "hi");
  EXPECT_EQ(e.expected_ack, 0xAABBCCDDu);
  EXPECT_EQ(e.direct_misses, 0);
  // first deadline = now + base (attempt 0 backoff == base)
  EXPECT_EQ(e.next_retry_at, 5000u + 2000u);
}

TEST(ChildOutbox, OnAckFreesMatchingSlotOnly) {
  ChildOutbox box;
  uint8_t a[6], b[6]; prefix(a, 1); prefix(b, 100);
  box.enqueue(a, "a", 1, 0, 0x11111111, 1000, 0);
  box.enqueue(b, "b", 2, 0, 0x22222222, 1000, 0);
  EXPECT_EQ(box.pendingCount(), 2);
  uint8_t ack[4] = {0x11, 0x11, 0x11, 0x11};
  EXPECT_TRUE(box.onAck(ack));
  EXPECT_EQ(box.pendingCount(), 1);
  // a second identical ACK matches nothing now
  EXPECT_FALSE(box.onAck(ack));
  EXPECT_EQ(box.pendingCount(), 1);
}

TEST(ChildOutbox, EvictsOldestWhenFull) {
  ChildOutbox box;
  uint8_t p[6];
  for (int i = 0; i < CHILD_OUTBOX_SLOTS; i++) {
    prefix(p, i); box.enqueue(p, "x", 100 + i, 0, 0x1000 + i, 1000, 0);
  }
  EXPECT_EQ(box.pendingCount(), CHILD_OUTBOX_SLOTS);
  // one more: oldest (smallest timestamp == 100) is evicted, count stays at cap
  prefix(p, 200); box.enqueue(p, "new", 999, 0, 0x9999, 1000, 0);
  EXPECT_EQ(box.pendingCount(), CHILD_OUTBOX_SLOTS);
  uint8_t old_ack[4] = {0x00, 0x10, 0x00, 0x00};  // 0x1000 little-endian
  EXPECT_FALSE(box.onAck(old_ack));  // evicted entry is gone
}

TEST(ChildOutbox, BackoffDoublesThenCaps) {
  EXPECT_EQ(ChildOutbox::nextBackoff(0, 2000), 2000u);
  EXPECT_EQ(ChildOutbox::nextBackoff(1, 2000), 4000u);
  EXPECT_EQ(ChildOutbox::nextBackoff(2, 2000), 8000u);
  EXPECT_EQ(ChildOutbox::nextBackoff(200, 2000), CHILD_OUTBOX_BACKOFF_CAP_MS);  // no overflow, capped
}

TEST(ChildOutbox, CollectDueReturnsOnlyExpiredEntries) {
  ChildOutbox box;
  uint8_t a[6] = {1,2,3,4,5,6}, b[6] = {9,9,9,9,9,9};
  box.enqueue(a, "a", 1, 0, 0xA, 2000, 0);      // due at 2000
  box.enqueue(b, "b", 2, 0, 0xB, 5000, 0);      // due at 5000
  int out[CHILD_OUTBOX_SLOTS];
  EXPECT_EQ(box.collectDue(1000, out), 0);      // neither due
  EXPECT_EQ(box.collectDue(2000, out), 1);      // 'a' due
  EXPECT_EQ(box.collectDue(9000, out), 2);      // both due
}

TEST(ChildOutbox, OnResentUpdatesAttemptAckDeadlineAndMisses) {
  ChildOutbox box;
  uint8_t a[6] = {1,2,3,4,5,6};
  box.enqueue(a, "a", 1, 0, 0xA, 2000, 0);
  box.onResent(0, 1, 0xBEEF, 3000, /*did_flood=*/false);
  const ChildOutboxEntry& e = box.at(0);
  EXPECT_EQ(e.attempt, 1);
  EXPECT_EQ(e.expected_ack, 0xBEEFu);
  EXPECT_EQ(e.last_send_at, 3000u);
  EXPECT_EQ(e.direct_misses, 1);               // a direct resend counts as a miss
  EXPECT_EQ(e.next_retry_at, 3000u + 4000u);   // now + nextBackoff(1, 2000)
  box.onResent(0, 2, 0xCAFE, 9000, /*did_flood=*/true);
  EXPECT_EQ(box.at(0).direct_misses, 0);       // flood resets the miss counter
}

TEST(ChildOutbox, RescheduleKeepsAttemptButPushesDeadline) {
  ChildOutbox box;
  uint8_t a[6] = {1,2,3,4,5,6};
  box.enqueue(a, "a", 1, 0, 0xA, 2000, 0);
  box.reschedule(0, 5000);
  const ChildOutboxEntry& e = box.at(0);
  EXPECT_EQ(e.attempt, 0);                      // unchanged
  EXPECT_EQ(e.next_retry_at, 5000u + 2000u);    // now + nextBackoff(0, 2000)
}

TEST(ChildOutbox, CollectNudgeRespectsMinGap) {
  ChildOutbox box;
  uint8_t a[6] = {1,2,3,4,5,6};
  box.enqueue(a, "a", 1, 0, 0xA, 60000, 0);   // long backoff: not "due" for a while
  int out[CHILD_OUTBOX_SLOTS];
  // too soon after the send -> not nudge-eligible
  EXPECT_EQ(box.collectNudge(CHILD_OUTBOX_NUDGE_MIN_GAP_MS - 1, out), 0);
  // past the min gap -> eligible even though the timed deadline is far off
  EXPECT_EQ(box.collectNudge(CHILD_OUTBOX_NUDGE_MIN_GAP_MS, out), 1);
  EXPECT_EQ(out[0], 0);
}

TEST(ChildOutbox, OnRecipientHeardMakesEntryDue) {
  ChildOutbox box;
  uint8_t a[6] = {1,2,3,4,5,6}, other[6] = {7,7,7,7,7,7};
  box.enqueue(a, "a", 1, 0, 0xA, 60000, 0);   // due far in the future
  int out[CHILD_OUTBOX_SLOTS];
  EXPECT_EQ(box.collectDue(1000, out), 0);
  EXPECT_FALSE(box.onRecipientHeard(other, 1000));  // no matching entry
  EXPECT_TRUE(box.onRecipientHeard(a, 1000));       // matches -> due now
  EXPECT_EQ(box.collectDue(1000, out), 1);
  EXPECT_EQ(out[0], 0);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
