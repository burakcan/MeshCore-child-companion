#include <gtest/gtest.h>
#include <string.h>
#include "helpers/child/ChildTextWrap.h"

static std::string seg(const char* text, const WrapLine& l) {
  return std::string(text + l.start, l.len);
}

TEST(ChildTextWrap, EmptyTextIsOneEmptyLine) {
  WrapLine lines[8];
  int n = childWrapLines("", 10, lines, 8);
  EXPECT_EQ(n, 1);
  EXPECT_EQ(lines[0].len, 0);
}

TEST(ChildTextWrap, ShortFitsOneLine) {
  WrapLine lines[8];
  const char* t = "hello";
  int n = childWrapLines(t, 10, lines, 8);
  EXPECT_EQ(n, 1);
  EXPECT_EQ(seg(t, lines[0]), "hello");
}

TEST(ChildTextWrap, WrapsOnSpace) {
  WrapLine lines[8];
  const char* t = "hello world";
  int n = childWrapLines(t, 7, lines, 8);
  EXPECT_EQ(n, 2);
  EXPECT_EQ(seg(t, lines[0]), "hello");
  EXPECT_EQ(seg(t, lines[1]), "world");
}

TEST(ChildTextWrap, HardBreaksLongWord) {
  WrapLine lines[8];
  const char* t = "abcdefghij";
  int n = childWrapLines(t, 4, lines, 8);
  EXPECT_EQ(n, 3);
  EXPECT_EQ(seg(t, lines[0]), "abcd");
  EXPECT_EQ(seg(t, lines[1]), "efgh");
  EXPECT_EQ(seg(t, lines[2]), "ij");
}

TEST(ChildTextWrap, NewlineForcesBreak) {
  WrapLine lines[8];
  const char* t = "a\nb";
  int n = childWrapLines(t, 10, lines, 8);
  EXPECT_EQ(n, 2);
  EXPECT_EQ(seg(t, lines[0]), "a");
  EXPECT_EQ(seg(t, lines[1]), "b");
}

TEST(ChildTextWrap, CapsAtMaxLines) {
  WrapLine lines[2];
  const char* t = "aa bb cc dd ee";
  int n = childWrapLines(t, 2, lines, 2);
  EXPECT_EQ(n, 2);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
