#include <gtest/gtest.h>
#include "helpers/ui/BatteryUtils.h"

TEST(BatteryUtils, FullAndEmpty) {
  EXPECT_EQ(batteryPercent(4200), 100);
  EXPECT_EQ(batteryPercent(3300), 0);
}
TEST(BatteryUtils, Midpoint) {
  EXPECT_EQ(batteryPercent(3750), 50);
}
TEST(BatteryUtils, Clamps) {
  EXPECT_EQ(batteryPercent(4500), 100);
  EXPECT_EQ(batteryPercent(3000), 0);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
