#include <gtest/gtest.h>
#include "helpers/ui/UiEmptyState.h"
#include "helpers/ui/UiIcons.h"
#include "FakeDisplayDriver.h"

TEST(UiEmptyState, DrawsIconAndLine) {
  FakeDisplayDriver d; d.startFrame();
  UiEmptyState::draw(d, ICON_ENVELOPE, "No messages yet");
  EXPECT_TRUE(d.printed("No messages yet"));
  bool has16 = false; for (auto& q : d.xbms) if (q[2]==16) has16=true;
  EXPECT_TRUE(has16);
}

int main(int argc, char** argv) { ::testing::InitGoogleTest(&argc, argv); return RUN_ALL_TESTS(); }
