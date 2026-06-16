#include <gtest/gtest.h>
#include "helpers/child/ChildPresetCommand.h"

TEST(ChildPresetCommand, ParsesSet) {
  int slot; char txt[CHILD_PRESET_TEXTLEN];
  EXPECT_TRUE(parsePresetCommand("!preset 3 On my way", slot, txt, sizeof(txt)));
  EXPECT_EQ(slot, 3);
  EXPECT_STREQ(txt, "On my way");
}

TEST(ChildPresetCommand, ParsesClearNoText) {
  int slot; char txt[CHILD_PRESET_TEXTLEN];
  EXPECT_TRUE(parsePresetCommand("!preset 3", slot, txt, sizeof(txt)));
  EXPECT_EQ(slot, 3);
  EXPECT_STREQ(txt, "");
}

TEST(ChildPresetCommand, ParsesClearTrailingSpaces) {
  int slot; char txt[CHILD_PRESET_TEXTLEN];
  EXPECT_TRUE(parsePresetCommand("!preset 3    ", slot, txt, sizeof(txt)));
  EXPECT_STREQ(txt, "");
}

TEST(ChildPresetCommand, ToleratesGroupPrefix) {
  int slot; char txt[CHILD_PRESET_TEXTLEN];
  EXPECT_TRUE(parsePresetCommand("Mom: !preset 2 Yes", slot, txt, sizeof(txt)));
  EXPECT_EQ(slot, 2);
  EXPECT_STREQ(txt, "Yes");
}

TEST(ChildPresetCommand, RejectsBadSlot) {
  int slot; char txt[CHILD_PRESET_TEXTLEN];
  EXPECT_FALSE(parsePresetCommand("!preset 0 x", slot, txt, sizeof(txt)));
  EXPECT_FALSE(parsePresetCommand("!preset 11 x", slot, txt, sizeof(txt)));
  EXPECT_FALSE(parsePresetCommand("!preset abc", slot, txt, sizeof(txt)));
}

TEST(ChildPresetCommand, RejectsNonCommand) {
  int slot; char txt[CHILD_PRESET_TEXTLEN];
  EXPECT_FALSE(parsePresetCommand("hello", slot, txt, sizeof(txt)));
  EXPECT_FALSE(parsePresetCommand("!pin 1 2", slot, txt, sizeof(txt)));
  EXPECT_FALSE(parsePresetCommand("!presetx 1 a", slot, txt, sizeof(txt)));
}

TEST(ChildPresetCommand, TruncatesToCap) {
  int slot; char txt[8];
  EXPECT_TRUE(parsePresetCommand("!preset 1 abcdefghijklmnop", slot, txt, sizeof(txt)));
  EXPECT_EQ((int)strlen(txt), 7);   // out_cap - 1
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
