#include <gtest/gtest.h>
#include "helpers/child/ChildCommands.h"

TEST(ChildCommands, ParsesPinChange) {
  PinChange pc;
  EXPECT_EQ(parseChildCommand("!pin 1234 5678", pc), CHILD_CMD_PIN_CHANGE);
  EXPECT_EQ(pc.old_pin, 1234u);
  EXPECT_EQ(pc.new_pin, 5678u);
}

TEST(ChildCommands, ParsesPinChangeWithGroupPrefix) {
  PinChange pc;
  EXPECT_EQ(parseChildCommand("Dad: !pin 1 9999", pc), CHILD_CMD_PIN_CHANGE);
  EXPECT_EQ(pc.old_pin, 1u);
  EXPECT_EQ(pc.new_pin, 9999u);
}

TEST(ChildCommands, IgnoresPlainText) {
  PinChange pc;
  EXPECT_EQ(parseChildCommand("hello there", pc), CHILD_CMD_NONE);
  EXPECT_EQ(parseChildCommand("pin 1 2", pc), CHILD_CMD_NONE);   // no leading '!'
}

TEST(ChildCommands, RejectsMalformedPin) {
  PinChange pc;
  EXPECT_EQ(parseChildCommand("!pin 1234", pc), CHILD_CMD_NONE);       // missing new
  EXPECT_EQ(parseChildCommand("!pin abc 5678", pc), CHILD_CMD_NONE);   // non-digit
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
