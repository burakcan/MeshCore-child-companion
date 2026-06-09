#include <gtest/gtest.h>
#include <string.h>
#include "helpers/child/ChildMessageStore.h"

TEST(ChildMessageStore, AddIsNewestFirst) {
  ChildMessageStore s;
  s.add("Mom", "first", 100, false);
  s.add("Dad", "second", 200, false);
  EXPECT_EQ(s.count(), 2);
  EXPECT_STREQ(s.at(0).text, "second");
  EXPECT_STREQ(s.at(0).origin, "Dad");
  EXPECT_STREQ(s.at(1).text, "first");
}

TEST(ChildMessageStore, RingWrapEvictsOldest) {
  ChildMessageStore s;
  char body[8];
  for (int i = 0; i <= CHILD_MSG_CAP; i++) {   // one more than capacity
    snprintf(body, sizeof(body), "m%d", i);
    s.add("X", body, (uint32_t)i, false);
  }
  EXPECT_EQ(s.count(), CHILD_MSG_CAP);
  EXPECT_STREQ(s.at(0).text, "m20");            // newest = last added
  EXPECT_STREQ(s.at(CHILD_MSG_CAP - 1).text, "m1");  // m0 evicted
}

TEST(ChildMessageStore, UnreadIncrementsAndClears) {
  ChildMessageStore s;
  s.add("A", "x", 1, false);
  s.add("B", "y", 2, false);
  EXPECT_EQ(s.unread(), 2);
  s.markAllRead();
  EXPECT_EQ(s.unread(), 0);
  EXPECT_EQ(s.count(), 2);                       // count unaffected
}

TEST(ChildMessageStore, BodyNotTruncatedAt160) {
  ChildMessageStore s;
  char body[161];
  memset(body, 'a', 160); body[160] = 0;
  s.add("A", body, 1, false);
  EXPECT_EQ((int)strlen(s.at(0).text), 160);
}

TEST(ChildMessageStore, LabelPrefixDmVsChannel) {
  ChildMessageStore s;
  s.add("Mom", "hi", 1, false);
  s.add("Family", "dinner", 2, true);
  const char* const* labels = s.labelPtrs();
  EXPECT_EQ(strncmp(labels[0], "(#) Family:", 11), 0);  // newest first, channel
  EXPECT_EQ(strncmp(labels[1], "(D) Mom:", 8), 0);       // DM
}

TEST(ChildMessageStore, LabelEllipsizedToWidth) {
  ChildMessageStore s;
  char body[200]; memset(body, 'z', 199); body[199] = 0;
  s.add("LongOriginNameThatIsAlsoVeryLong", body, 1, false);
  const char* const* labels = s.labelPtrs();
  EXPECT_LE((int)strlen(labels[0]), CHILD_MSG_LABEL - 1);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
