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

TEST(ChildSenderApproved, AnonTypeRejected) {
  EXPECT_FALSE(childSenderApproved(0));   // ADV_TYPE_NONE
}

TEST(ChildSenderApproved, RealTypesApproved) {
  EXPECT_TRUE(childSenderApproved(1));    // ADV_TYPE_CHAT
  EXPECT_TRUE(childSenderApproved(2));    // ADV_TYPE_REPEATER
  EXPECT_TRUE(childSenderApproved(3));    // ADV_TYPE_ROOM
  EXPECT_TRUE(childSenderApproved(4));    // ADV_TYPE_SENSOR
}

TEST(TzCommand, ParsesPositive) {
  int m = 999;
  EXPECT_TRUE(parseTzCommand("!tz 60", &m));
  EXPECT_EQ(m, 60);
}
TEST(TzCommand, ParsesNegative) {
  int m = 0;
  EXPECT_TRUE(parseTzCommand("!tz -300", &m));
  EXPECT_EQ(m, -300);
}
TEST(TzCommand, GroupPrefixTolerated) {
  int m = 0;
  EXPECT_TRUE(parseTzCommand("Dad: !tz 120", &m));
  EXPECT_EQ(m, 120);
}
TEST(TzCommand, RejectsOutOfRange) {
  int m = 0;
  EXPECT_FALSE(parseTzCommand("!tz 9000", &m));
}
TEST(TzCommand, RejectsNonTz) {
  int m = 0;
  EXPECT_FALSE(parseTzCommand("hello", &m));
  EXPECT_FALSE(parseTzCommand("!tz", &m));        // no argument
  EXPECT_FALSE(parseTzCommand("!tz abc", &m));
}

TEST(NameCommand, ParsesName) {
  char nm[32] = "";
  EXPECT_TRUE(parseNameCommand("!name Grandma", nm, sizeof(nm)));
  EXPECT_STREQ(nm, "Grandma");
}
TEST(NameCommand, GroupPrefixTolerated) {
  char nm[32] = "";
  EXPECT_TRUE(parseNameCommand("Bob: !name Uncle Bob", nm, sizeof(nm)));
  EXPECT_STREQ(nm, "Uncle Bob");
}
TEST(NameCommand, TrimsTrailingSpaces) {
  char nm[32] = "";
  EXPECT_TRUE(parseNameCommand("!name Mia   ", nm, sizeof(nm)));
  EXPECT_STREQ(nm, "Mia");
}
TEST(NameCommand, RejectsEmptyOrNonName) {
  char nm[32] = "x";
  EXPECT_FALSE(parseNameCommand("!name ", nm, sizeof(nm)));
  EXPECT_FALSE(parseNameCommand("hello", nm, sizeof(nm)));
}

TEST(ChildCommands, ParseRetryOnOff) {
  bool en = false;
  EXPECT_TRUE(parseRetryCommand("!retry on", &en));
  EXPECT_TRUE(en);
  EXPECT_TRUE(parseRetryCommand("!retry off", &en));
  EXPECT_FALSE(en);
}

TEST(ChildCommands, ParseRetryToleratesGroupPrefix) {
  bool en = true;
  EXPECT_TRUE(parseRetryCommand("Mum: !retry off", &en));
  EXPECT_FALSE(en);
}

TEST(ChildCommands, ParseRetryRejectsJunk) {
  bool en = true;
  EXPECT_FALSE(parseRetryCommand("!retry maybe", &en));   // unknown arg
  EXPECT_FALSE(parseRetryCommand("!retry onx", &en));     // trailing junk on "on"
  EXPECT_FALSE(parseRetryCommand("!retr on", &en));       // wrong command
  EXPECT_FALSE(parseRetryCommand("hello", &en));          // not a command
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
