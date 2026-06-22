#include <gtest/gtest.h>
#include "helpers/ui/UiFooter.h"
#include "helpers/ui/UiIcons.h"
#include "FakeDisplayDriver.h"

TEST(UiFooter, DrawsIconAndText) {
  FakeDisplayDriver d; d.startFrame();
  UiFooter::hint(d, "menu", ICON_BTN);
  EXPECT_TRUE(d.printed("menu"));
  bool has8 = false; for (auto& q : d.xbms) if (q[2]==8) has8=true;
  EXPECT_TRUE(has8);
}

int main(int argc, char** argv) { ::testing::InitGoogleTest(&argc, argv); return RUN_ALL_TESTS(); }
