#include <gtest/gtest.h>
#include "helpers/ui/UiIcons.h"

TEST(UiIcons, EveryIdResolves) {
  for (int i = 0; i < UI_ICON_COUNT; i++) {
    int w = 0, h = 0;
    const uint8_t* b = uiIcon((UiIconId)i, &w, &h);
    ASSERT_NE(b, nullptr) << "id " << i;
    EXPECT_TRUE((w == 16 && h == 16) || (w == 8 && h == 8)) << "id " << i << " dims " << w << "x" << h;
  }
}

TEST(UiIcons, OutOfRangeNull) {
  int w = 1, h = 1;
  EXPECT_EQ(uiIcon((UiIconId)UI_ICON_COUNT, &w, &h), nullptr);
  EXPECT_EQ(w, 0); EXPECT_EQ(h, 0);
}

int main(int argc, char** argv) { ::testing::InitGoogleTest(&argc, argv); return RUN_ALL_TESTS(); }
