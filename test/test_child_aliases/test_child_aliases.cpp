#include <gtest/gtest.h>
#include <string.h>
#include "helpers/child/ChildAliases.h"

TEST(ChildAliases, SetAndLookup) {
  ChildAliases a;
  a.set("CF01", "burak");
  EXPECT_STREQ(a.lookup("CF01"), "burak");
  EXPECT_EQ(a.lookup("nobody"), nullptr);
}

TEST(ChildAliases, ReRenameKeepsOneEntry) {
  ChildAliases a;
  a.set("CF01", "burak");
  a.set("burak", "bob");                 // rename again using the current display name
  EXPECT_EQ(a.count(), 1);
  EXPECT_STREQ(a.lookup("CF01"), "bob");  // original device name still maps, now to bob
}

TEST(ChildAliases, UpdateExistingKey) {
  ChildAliases a;
  a.set("CF01", "burak");
  a.set("CF01", "dad");
  EXPECT_EQ(a.count(), 1);
  EXPECT_STREQ(a.lookup("CF01"), "dad");
}

TEST(ChildAliases, PackUnpackRoundTrip) {
  ChildAliases a;
  a.set("CF01", "burak");
  a.set("AB12", "mom");
  uint8_t buf[CHILD_ALIAS_SAVE_LEN];
  int n = a.pack(buf);
  EXPECT_EQ(n, CHILD_ALIAS_SAVE_LEN);

  ChildAliases b;
  EXPECT_TRUE(b.unpack(buf, n));
  EXPECT_EQ(b.count(), 2);
  EXPECT_STREQ(b.lookup("CF01"), "burak");
  EXPECT_STREQ(b.lookup("AB12"), "mom");
}

TEST(ChildAliases, IgnoresEmpty) {
  ChildAliases a;
  a.set("", "x");
  a.set("y", "");
  EXPECT_EQ(a.count(), 0);
}

int main(int argc, char** argv) { ::testing::InitGoogleTest(&argc, argv); return RUN_ALL_TESTS(); }
