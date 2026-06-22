#include <gtest/gtest.h>
#include <string.h>
#include "helpers/child/ChildMessageStore.h"

static const uint8_t PFX[6] = {1, 2, 3, 4, 5, 6};

TEST(ChildMessageStore, AddIsNewestFirst) {
  ChildMessageStore s;
  s.add("Mom", "first", 100, false, PFX, 0xFF, false);
  s.add("Dad", "second", 200, false, PFX, 0xFF, false);
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
    s.add("X", body, (uint32_t)i, false, PFX, 0xFF, false);
  }
  EXPECT_EQ(s.count(), CHILD_MSG_CAP);
  EXPECT_STREQ(s.at(0).text, "m20");
  EXPECT_STREQ(s.at(CHILD_MSG_CAP - 1).text, "m1");
}

TEST(ChildMessageStore, UnreadCountsUnreadMessages) {
  ChildMessageStore s;
  s.add("A", "x", 1, false, PFX, 0xFF, false);
  s.add("B", "y", 2, false, PFX, 0xFF, false);
  s.add("C", "z", 3, false, PFX, 0xFF, false);
  EXPECT_EQ(s.unread(), 3);
  s.markRead(0);
  EXPECT_EQ(s.unread(), 2);
  s.markRead(2);
  EXPECT_EQ(s.unread(), 1);
  EXPECT_EQ(s.count(), 3);
}

TEST(ChildMessageStore, MarkReadTransitionsOnlyOnce) {
  ChildMessageStore s;
  s.add("A", "x", 1, false, PFX, 0xFF, false);
  EXPECT_TRUE(s.markRead(0).transitioned);
  EXPECT_FALSE(s.markRead(0).transitioned);
}

TEST(ChildMessageStore, MarkReadReturnsDmIdentity) {
  ChildMessageStore s;
  s.add("Mom", "hi", 1, false, PFX, 0xFF, false);
  ChildReadResult r = s.markRead(0);
  EXPECT_TRUE(r.transitioned);
  EXPECT_FALSE(r.is_channel);
  EXPECT_EQ(memcmp(r.sender, PFX, 6), 0);
}

TEST(ChildMessageStore, MarkReadReturnsChannelIdentity) {
  ChildMessageStore s;
  s.add("Family", "dinner", 1, true, nullptr, 7, false);
  ChildReadResult r = s.markRead(0);
  EXPECT_TRUE(r.is_channel);
  EXPECT_EQ(r.channel_idx, 7);
}

TEST(ChildMessageStore, BodyNotTruncatedAt160) {
  ChildMessageStore s;
  char body[161];
  memset(body, 'a', 160); body[160] = 0;
  s.add("A", body, 1, false, PFX, 0xFF, false);
  EXPECT_EQ((int)strlen(s.at(0).text), 160);
}

TEST(ChildMessageStore, LabelMarksUnreadThenClears) {
  ChildMessageStore s;
  s.add("Mom", "hi", 1, false, PFX, 0xFF, false);
  const char* const* labels = s.labelPtrs();
  EXPECT_EQ(strncmp(labels[0], "* Mom:", 6), 0);
  s.markRead(0);
  labels = s.labelPtrs();
  EXPECT_EQ(strncmp(labels[0], "Mom:", 4), 0);
}

TEST(ChildMessageStore, LabelPrefixChannel) {
  ChildMessageStore s;
  s.add("Family", "dinner", 1, true, nullptr, 3, false);
  const char* const* labels = s.labelPtrs();
  EXPECT_EQ(strncmp(labels[0], "* Family:", 9), 0);
}

TEST(ChildMessageStore, LabelEllipsizedToWidth) {
  ChildMessageStore s;
  char body[200]; memset(body, 'z', 199); body[199] = 0;
  s.add("LongOriginNameThatIsAlsoVeryLong", body, 1, false, PFX, 0xFF, false);
  const char* const* labels = s.labelPtrs();
  EXPECT_LE((int)strlen(labels[0]), CHILD_MSG_LABEL - 1);
}

TEST(ChildMessageStore, QuestionPendingUntilAnswered) {
  ChildMessageStore s;
  s.add("Mom", "?dinner | a | b", 1, false, PFX, 0xFF, true);
  EXPECT_EQ(s.unread(), 1);          // unanswered question is pending
  s.markAnswered(0, 1);
  EXPECT_EQ(s.unread(), 0);          // answered -> not pending
  EXPECT_EQ(s.count(), 1);
}

TEST(ChildMessageStore, LabelMarksQuestionThenClears) {
  ChildMessageStore s;
  s.add("Mom", "?dinner | a | b", 1, false, PFX, 0xFF, true);
  const char* const* labels = s.labelPtrs();
  EXPECT_EQ(strncmp(labels[0], "? Mom:", 6), 0);   // needs-answer marker
  s.markAnswered(0, 0);
  labels = s.labelPtrs();
  EXPECT_EQ(strncmp(labels[0], "Mom:", 4), 0);       // marker gone after answer
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
