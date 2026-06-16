#include <gtest/gtest.h>
#include <string.h>
#include "helpers/child/ChildPresets.h"

TEST(ChildPresets, DefaultsSeedFirstFive) {
  ChildPresets p;
  p.initDefaults();
  EXPECT_STREQ(p.text(0), "I'm OK");
  EXPECT_STREQ(p.text(1), "On my way");
  EXPECT_STREQ(p.text(2), "Pick me up");
  EXPECT_STREQ(p.text(3), "Yes");
  EXPECT_STREQ(p.text(4), "No");
  EXPECT_TRUE(p.isEmpty(5));
  EXPECT_TRUE(p.isEmpty(9));
  EXPECT_EQ(p.labelCount(), 5);
}

TEST(ChildPresets, SetAndClear) {
  ChildPresets p;
  p.initDefaults();
  EXPECT_TRUE(p.set(5, "Be there soon"));
  EXPECT_FALSE(p.isEmpty(5));
  EXPECT_STREQ(p.text(5), "Be there soon");
  p.clear(5);
  EXPECT_TRUE(p.isEmpty(5));
  EXPECT_STREQ(p.text(5), "");
}

TEST(ChildPresets, SetEmptyClears) {
  ChildPresets p;
  p.set(0, "x");
  EXPECT_TRUE(p.set(0, ""));      // empty text clears
  EXPECT_TRUE(p.isEmpty(0));
  EXPECT_TRUE(p.set(1, nullptr)); // null clears too
  EXPECT_TRUE(p.isEmpty(1));
}

TEST(ChildPresets, BoundsRejected) {
  ChildPresets p;
  EXPECT_FALSE(p.set(-1, "x"));
  EXPECT_FALSE(p.set(CHILD_PRESET_SLOTS, "x"));
  EXPECT_TRUE(p.isEmpty(-1));
  EXPECT_STREQ(p.text(99), "");
}

TEST(ChildPresets, TruncatesToMax) {
  ChildPresets p;
  char big[200];
  memset(big, 'a', sizeof(big));
  big[sizeof(big) - 1] = 0;
  p.set(0, big);
  EXPECT_EQ((int)strlen(p.text(0)), CHILD_PRESET_TEXTLEN - 1);  // 80 chars
}

TEST(ChildPresets, PackUnpackRoundTrip) {
  ChildPresets a;
  a.set(0, "Hello there");
  uint8_t buf[CHILD_PRESET_TEXTLEN + 2];
  int n = a.packSlot(0, buf);
  EXPECT_EQ(n, 2 + (int)strlen("Hello there"));
  ChildPresets b;
  EXPECT_TRUE(b.unpackSlot(0, buf, n));
  EXPECT_STREQ(b.text(0), "Hello there");
}

TEST(ChildPresets, PackUnpackEmptySlot) {
  ChildPresets a;            // slot 0 empty
  uint8_t buf[CHILD_PRESET_TEXTLEN + 2];
  int n = a.packSlot(0, buf);
  EXPECT_EQ(n, 2);           // version + len(0)
  ChildPresets b;
  b.set(0, "stale");
  EXPECT_TRUE(b.unpackSlot(0, buf, n));
  EXPECT_TRUE(b.isEmpty(0));
}

TEST(ChildPresets, UnpackRejectsBadVersionOrShort) {
  ChildPresets p;
  uint8_t bad[4] = { 99, 1, 'x', 0 };
  EXPECT_FALSE(p.unpackSlot(0, bad, 4));  // wrong version
  uint8_t one[1] = { CHILD_PRESET_VERSION };
  EXPECT_FALSE(p.unpackSlot(0, one, 1));  // too short
}

TEST(ChildPresets, LabelMappingSkipsEmpties) {
  ChildPresets p;
  p.initDefaults();          // slots 0..4 filled
  p.set(7, "Custom");        // gap at 5,6
  p.labelPtrs();
  EXPECT_EQ(p.labelCount(), 6);
  EXPECT_EQ(p.slotForRow(0), 0);
  EXPECT_EQ(p.slotForRow(4), 4);
  EXPECT_EQ(p.slotForRow(5), 7);   // the custom one
  EXPECT_EQ(p.slotForRow(6), -1);  // out of range
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
