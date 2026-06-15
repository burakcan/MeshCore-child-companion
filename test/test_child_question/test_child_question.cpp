#include <gtest/gtest.h>
#include <string.h>
#include "helpers/child/ChildQuestion.h"

TEST(ChildQuestion, ValidDm) {
  ChildQuestion q;
  EXPECT_TRUE(parseChildQuestion("?dinner | pizza | pasta", q));
  EXPECT_STREQ(q.question, "dinner");
  EXPECT_EQ(q.num_options, 2);
  EXPECT_STREQ(q.options[0], "pizza");
  EXPECT_STREQ(q.options[1], "pasta");
}

TEST(ChildQuestion, GroupPrefixStripped) {
  ChildQuestion q;
  EXPECT_TRUE(parseChildQuestion("Mom: ?dinner | pizza | pasta", q));
  EXPECT_STREQ(q.question, "dinner");
  EXPECT_EQ(q.num_options, 2);
}

TEST(ChildQuestion, TrimsWhitespace) {
  ChildQuestion q;
  EXPECT_TRUE(parseChildQuestion("?  where  |  home  |  park  ", q));
  EXPECT_STREQ(q.question, "where");
  EXPECT_STREQ(q.options[0], "home");
  EXPECT_STREQ(q.options[1], "park");
}

TEST(ChildQuestion, NotAQuestionPlainText) {
  ChildQuestion q;
  EXPECT_FALSE(parseChildQuestion("hello there", q));
}

TEST(ChildQuestion, NotAQuestionNoOptions) {
  ChildQuestion q;
  EXPECT_FALSE(parseChildQuestion("?just a question", q));
}

TEST(ChildQuestion, OneOptionRejected) {
  ChildQuestion q;
  EXPECT_FALSE(parseChildQuestion("?ok | yes", q));
}

TEST(ChildQuestion, EmptyOptionsDropped) {
  ChildQuestion q;
  EXPECT_TRUE(parseChildQuestion("?q | a |  | b", q));
  EXPECT_EQ(q.num_options, 2);
  EXPECT_STREQ(q.options[0], "a");
  EXPECT_STREQ(q.options[1], "b");
}

TEST(ChildQuestion, CapsAtMaxOptions) {
  ChildQuestion q;
  EXPECT_TRUE(parseChildQuestion("?q | a | b | c | d | e | f | g", q));
  EXPECT_EQ(q.num_options, CHILD_Q_MAXOPTS);   // 6, extras ignored
}

TEST(ChildQuestion, TruncatesLongOption) {
  ChildQuestion q;
  char big[80]; memset(big, 'z', 79); big[79] = 0;
  char msg[120];
  snprintf(msg, sizeof(msg), "?q | %s | b", big);
  EXPECT_TRUE(parseChildQuestion(msg, q));
  EXPECT_LE((int)strlen(q.options[0]), CHILD_Q_OPTLEN - 1);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
