#include <gtest/gtest.h>
#include <string.h>
#include "helpers/child/ChildMessageStore.h"

static const uint8_t PFX[6] = {1, 2, 3, 4, 5, 6};

TEST(ChildMessageStore, AddIsNewestFirst) {
  ChildMessageStore s;
  s.add("Mom", "first", 100, false, PFX, 0xFF);
  s.add("Dad", "second", 200, false, PFX, 0xFF);
  EXPECT_EQ(s.count(), 2);
  EXPECT_STREQ(s.at(0).text, "second");
  EXPECT_STREQ(s.at(0).origin, "Dad");
  EXPECT_STREQ(s.at(1).text, "first");
}

TEST(ChildMessageStore, RingWrapEvictsOldest) {
  ChildMessageStore s;
  char body[8];
  for (int i = 0; i <= CHILD_MSG_CAP; i++) {
    snprintf(body, sizeof(body), "m%d", i);
    s.add("X", body, (uint32_t)i, false, PFX, 0xFF);
  }
  EXPECT_EQ(s.count(), CHILD_MSG_CAP);
  EXPECT_STREQ(s.at(0).text, "m20");
  EXPECT_STREQ(s.at(CHILD_MSG_CAP - 1).text, "m1");
}

TEST(ChildMessageStore, UnreadCountsUnreadMessages) {
  ChildMessageStore s;
  s.add("A", "x", 1, false, PFX, 0xFF);
  s.add("B", "y", 2, false, PFX, 0xFF);
  s.add("C", "z", 3, false, PFX, 0xFF);
  EXPECT_EQ(s.unread(), 3);
  s.markRead(0);            // newest (C)
  EXPECT_EQ(s.unread(), 2);
  s.markRead(2);            // oldest (A)
  EXPECT_EQ(s.unread(), 1);
  EXPECT_EQ(s.count(), 3);  // count unaffected
}

TEST(ChildMessageStore, MarkReadTransitionsOnlyOnce) {
  ChildMessageStore s;
  s.add("A", "x", 1, false, PFX, 0xFF);
  ChildReadResult r1 = s.markRead(0);
  EXPECT_TRUE(r1.transitioned);
  ChildReadResult r2 = s.markRead(0);
  EXPECT_FALSE(r2.transitioned);
}

TEST(ChildMessageStore, MarkReadReturnsDmIdentity) {
  ChildMessageStore s;
  s.add("Mom", "hi", 1, false, PFX, 0xFF);
  ChildReadResult r = s.markRead(0);
  EXPECT_TRUE(r.transitioned);
  EXPECT_FALSE(r.is_channel);
  EXPECT_EQ(memcmp(r.sender, PFX, 6), 0);
}

TEST(ChildMessageStore, MarkReadReturnsChannelIdentity) {
  ChildMessageStore s;
  s.add("Family", "dinner", 1, true, nullptr, 7);
  ChildReadResult r = s.markRead(0);
  EXPECT_TRUE(r.transitioned);
  EXPECT_TRUE(r.is_channel);
  EXPECT_EQ(r.channel_idx, 7);
}

TEST(ChildMessageStore, BodyNotTruncatedAt160) {
  ChildMessageStore s;
  char body[161];
  memset(body, 'a', 160); body[160] = 0;
  s.add("A", body, 1, false, PFX, 0xFF);
  EXPECT_EQ((int)strlen(s.at(0).text), 160);
}

TEST(ChildMessageStore, LabelMarksUnreadThenClears) {
  ChildMessageStore s;
  s.add("Mom", "hi", 1, false, PFX, 0xFF);
  const char* const* labels = s.labelPtrs();
  EXPECT_EQ(strncmp(labels[0], "* (D) Mom:", 10), 0);  // unread marker present
  s.markRead(0);
  labels = s.labelPtrs();
  EXPECT_EQ(strncmp(labels[0], "(D) Mom:", 8), 0);      // marker gone after read
}

TEST(ChildMessageStore, LabelPrefixChannel) {
  ChildMessageStore s;
  s.add("Family", "dinner", 1, true, nullptr, 3);
  const char* const* labels = s.labelPtrs();
  EXPECT_EQ(strncmp(labels[0], "* (#) Family:", 13), 0);
}

TEST(ChildMessageStore, LabelEllipsizedToWidth) {
  ChildMessageStore s;
  char body[200]; memset(body, 'z', 199); body[199] = 0;
  s.add("LongOriginNameThatIsAlsoVeryLong", body, 1, false, PFX, 0xFF);
  const char* const* labels = s.labelPtrs();
  EXPECT_LE((int)strlen(labels[0]), CHILD_MSG_LABEL - 1);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
