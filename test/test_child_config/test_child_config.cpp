#include <gtest/gtest.h>
#include "helpers/child/ChildConfig.h"

TEST(ChildConfig, InitSetsVersionAndPin) {
  ChildConfig cfg;
  childConfigInit(cfg, 1234);
  EXPECT_EQ(cfg.version, CHILD_CONFIG_VERSION);
  EXPECT_EQ(cfg.pin, 1234u);
}

TEST(ChildConfig, PackUnpackRoundTrip) {
  ChildConfig in; childConfigInit(in, 8765);
  uint8_t buf[16];
  int n = childConfigPack(in, buf);
  EXPECT_EQ(n, CHILD_CONFIG_BLOB_LEN);

  ChildConfig out;
  EXPECT_TRUE(childConfigUnpack(out, buf, n));
  EXPECT_EQ(out.version, in.version);
  EXPECT_EQ(out.pin, 8765u);
}

TEST(ChildConfig, UnpackRejectsShortBuffer) {
  ChildConfig out;
  uint8_t buf[2] = {1, 0};
  EXPECT_FALSE(childConfigUnpack(out, buf, 2));
}

TEST(ChildConfig, UnpackRejectsUnknownVersion) {
  ChildConfig out;
  uint8_t buf[CHILD_CONFIG_BLOB_LEN] = {99, 1, 0, 0, 0};
  EXPECT_FALSE(childConfigUnpack(out, buf, CHILD_CONFIG_BLOB_LEN));
}

TEST(ChildConfig, TimezoneRoundTrip) {
  ChildConfig in; childConfigInit(in, 1111);
  in.tz_offset_min = -300;                   // UTC-5
  uint8_t buf[16];
  int n = childConfigPack(in, buf);
  ChildConfig out;
  EXPECT_TRUE(childConfigUnpack(out, buf, n));
  EXPECT_EQ(out.tz_offset_min, -300);
  EXPECT_EQ(out.pin, 1111u);
}

TEST(ChildConfig, MigratesV1BlobToTzZero) {
  // an old v1 blob: version 1 + pin, no timezone bytes
  uint8_t buf[5] = {1, 0xD2, 0x04, 0, 0};    // pin = 1234
  ChildConfig out;
  EXPECT_TRUE(childConfigUnpack(out, buf, 5));
  EXPECT_EQ(out.pin, 1234u);
  EXPECT_EQ(out.tz_offset_min, 0);
  EXPECT_EQ(out.version, CHILD_CONFIG_VERSION);   // upgraded in memory
}

TEST(ChildConfig, UnpackReadsFromZeroPaddedBuffer) {
  ChildConfig in; childConfigInit(in, 4321);
  uint8_t buf[100];
  memset(buf, 0, sizeof(buf));
  childConfigPack(in, buf);                 // writes first 5 bytes, rest stays 0
  ChildConfig out;
  EXPECT_TRUE(childConfigUnpack(out, buf, 100));  // reads first 5, ignores padding
  EXPECT_EQ(out.pin, 4321u);
  EXPECT_EQ(out.version, CHILD_CONFIG_VERSION);
}

TEST(ChildConfig, RetryEnabledDefaultsTrue) {
  ChildConfig cfg; childConfigInit(cfg, 1);
  EXPECT_TRUE(cfg.retry_enabled);
}

TEST(ChildConfig, RetryEnabledRoundTrip) {
  ChildConfig in; childConfigInit(in, 1);
  in.retry_enabled = false;
  uint8_t buf[16];
  int n = childConfigPack(in, buf);
  EXPECT_EQ(n, CHILD_CONFIG_BLOB_LEN);          // v3 = 8 bytes
  ChildConfig out;
  EXPECT_TRUE(childConfigUnpack(out, buf, n));
  EXPECT_FALSE(out.retry_enabled);
  EXPECT_EQ(out.version, CHILD_CONFIG_VERSION);
}

TEST(ChildConfig, MigratesV2BlobToRetryEnabled) {
  // a v2 blob: version 2 + pin + tz, no retry byte
  uint8_t buf[7] = {2, 0xD2, 0x04, 0, 0, 0x3C, 0x00};  // pin=1234, tz=+60
  ChildConfig out;
  EXPECT_TRUE(childConfigUnpack(out, buf, 7));
  EXPECT_EQ(out.pin, 1234u);
  EXPECT_EQ(out.tz_offset_min, 60);
  EXPECT_TRUE(out.retry_enabled);              // default on for migrated blobs
  EXPECT_EQ(out.version, CHILD_CONFIG_VERSION);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
