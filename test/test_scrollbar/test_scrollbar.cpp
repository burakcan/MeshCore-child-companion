#include <gtest/gtest.h>
#include "helpers/ui/Scrollbar.h"

TEST(Scrollbar, HiddenWhenFits) {
  auto t = Scrollbar::thumbRect(0, 50, 3, 4, 0);   // total <= visible
  EXPECT_FALSE(t.visible);
}

TEST(Scrollbar, TopOffset) {
  auto t = Scrollbar::thumbRect(0, 50, 10, 5, 0);   // half the list visible, at top
  EXPECT_TRUE(t.visible);
  EXPECT_EQ(t.y, 0);
  EXPECT_EQ(t.h, 25);                                // 5/10 * 50
}

TEST(Scrollbar, BottomOffsetClamps) {
  auto t = Scrollbar::thumbRect(0, 50, 10, 5, 5);   // scrolled to bottom
  EXPECT_TRUE(t.visible);
  EXPECT_EQ(t.y + t.h, 50);                          // thumb bottom flush with track bottom
}

int main(int argc, char** argv) { ::testing::InitGoogleTest(&argc, argv); return RUN_ALL_TESTS(); }
