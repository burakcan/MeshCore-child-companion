#include <gtest/gtest.h>
#include <string.h>
#include "helpers/child/ChildBell.h"

#define BELL "\xF0\x9F\x94\x94"   // U+1F514

TEST(ChildBell, BareEmoji)            { EXPECT_TRUE(parseChildBell(BELL)); }
TEST(ChildBell, GroupPrefix)          { EXPECT_TRUE(parseChildBell("Mom: " BELL)); }
TEST(ChildBell, LeadingTrailingSpace) { EXPECT_TRUE(parseChildBell("  " BELL "  ")); }
TEST(ChildBell, GroupPrefixSpaces)    { EXPECT_TRUE(parseChildBell("Dad:  " BELL)); }

TEST(ChildBell, EmojiWithTrailing)    { EXPECT_FALSE(parseChildBell(BELL " come home")); }
TEST(ChildBell, MidSentenceNoPrefix)  { EXPECT_FALSE(parseChildBell("ring " BELL " now")); }
TEST(ChildBell, GroupMidSentence)     { EXPECT_FALSE(parseChildBell("Dad: come home " BELL)); }
TEST(ChildBell, PlainText)            { EXPECT_FALSE(parseChildBell("hello")); }
TEST(ChildBell, DifferentEmoji)       { EXPECT_FALSE(parseChildBell("\xF0\x9F\x93\x8D")); } // 📍
TEST(ChildBell, Empty)                { EXPECT_FALSE(parseChildBell("")); }
TEST(ChildBell, Null)                 { EXPECT_FALSE(parseChildBell(NULL)); }

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
