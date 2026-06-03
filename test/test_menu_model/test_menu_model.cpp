#include <gtest/gtest.h>
#include "helpers/ui/MenuModel.h"

TEST(MenuModel, StartsAtZero) {
  MenuModel m; m.reset(5, 3);
  EXPECT_EQ(m.selected(), 0);
  EXPECT_EQ(m.top(), 0);
}

TEST(MenuModel, NextAdvancesAndWraps) {
  MenuModel m; m.reset(3, 3);
  m.next(); EXPECT_EQ(m.selected(), 1);
  m.next(); EXPECT_EQ(m.selected(), 2);
  m.next(); EXPECT_EQ(m.selected(), 0);   // wrap
}

TEST(MenuModel, PrevWrapsBackward) {
  MenuModel m; m.reset(3, 3);
  m.prev(); EXPECT_EQ(m.selected(), 2);   // wrap
}

TEST(MenuModel, ScrollWindowFollowsSelection) {
  MenuModel m; m.reset(5, 3);             // rows visible: 3
  m.next(); m.next(); m.next();           // selected = 3, must scroll
  EXPECT_EQ(m.selected(), 3);
  EXPECT_EQ(m.top(), 1);                  // window [1..3]
  EXPECT_LE(m.top(), m.selected());
  EXPECT_LT(m.selected(), m.top() + 3);
}

TEST(MenuModel, EmptyIsSafe) {
  MenuModel m; m.reset(0, 3);
  m.next(); m.prev();                     // no crash, no movement
  EXPECT_EQ(m.selected(), 0);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
