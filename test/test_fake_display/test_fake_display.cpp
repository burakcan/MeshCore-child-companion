#include <gtest/gtest.h>
#include "FakeDisplayDriver.h"

TEST(FakeDisplayDriver, InstantiatesAndRecordsFrame) {
  FakeDisplayDriver d;
  d.startFrame();
  EXPECT_EQ(d.frames, 1);
}

TEST(FakeDisplayDriver, PrintRecorded) {
  FakeDisplayDriver d;
  d.startFrame();
  d.print("hi");
  EXPECT_TRUE(d.printed("hi"));
}

TEST(FakeDisplayDriver, DrawTextCenteredRecordsText) {
  FakeDisplayDriver d;
  d.startFrame();
  d.drawTextCentered(64, 0, "hi");
  EXPECT_TRUE(d.printed("hi"));
}

TEST(FakeDisplayDriver, StartFrameClearsTexts) {
  FakeDisplayDriver d;
  d.startFrame();
  d.print("old");
  d.startFrame();
  EXPECT_FALSE(d.printed("old"));
  EXPECT_EQ(d.frames, 2);
}

TEST(FakeDisplayDriver, TurnOnOffTracked) {
  FakeDisplayDriver d;
  EXPECT_TRUE(d.isOn());
  d.turnOff();
  EXPECT_FALSE(d.isOn());
  d.turnOn();
  EXPECT_TRUE(d.isOn());
}

TEST(FakeDisplayDriver, FillRectCounted) {
  FakeDisplayDriver d;
  d.startFrame();
  d.fillRect(0, 0, 10, 10);
  d.fillRect(5, 5, 20, 20);
  EXPECT_EQ(d.fills, 2);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
