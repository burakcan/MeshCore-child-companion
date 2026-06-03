#include <gtest/gtest.h>
#include "helpers/ui/StatusHeader.h"
#include "FakeDisplayDriver.h"

TEST(StatusHeader, DrawsNameAndBattery) {
  FakeDisplayDriver d; d.startFrame();
  StatusHeader::draw(d, "Mia", "14:05", 78, true);
  EXPECT_TRUE(d.printed("Mia"));
  EXPECT_TRUE(d.printed("78"));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
