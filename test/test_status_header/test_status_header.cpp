#include <gtest/gtest.h>
#include "helpers/ui/StatusHeader.h"
#include "helpers/ui/UiIcons.h"
#include "FakeDisplayDriver.h"

TEST(StatusHeader, DrawsNameAndBattery) {
  FakeDisplayDriver d; d.startFrame();
  StatusHeader::draw(d, "Mia", "14:05", 78, true);
  EXPECT_TRUE(d.printed("Mia"));
  EXPECT_FALSE(d.printed("78%"));   // battery is now an icon, not % text
}

// A leading icon draws a 16x16 XBM; the battery is an icon (not "%") rather than text.
TEST(StatusHeader, DrawsLeadingIconAndBattery) {
  FakeDisplayDriver d; d.startFrame();
  StatusHeader::draw(d, "Mia", "14:05", 80, true, ICON_BELL);
  bool has16 = false;
  for (auto& q : d.xbms) if (q[2]==16) has16=true;
  EXPECT_TRUE(has16);   // the bell lead icon
  EXPECT_TRUE(d.printed("Mia"));
  EXPECT_FALSE(d.printed("80%"));
}

// batt_pct < 0 means "no battery info": no battery outline drawn.
TEST(StatusHeader, NegativeBatteryNotDrawn) {
  FakeDisplayDriver d; d.startFrame();
  StatusHeader::draw(d, "Mia", "", -1, true, ICON_BELL);
  // only the separator drawRect; battery outline would add another rect at top-right
  EXPECT_EQ(d.rects, 1);
}

// SH1106 renders CP437, not UTF-8: a name with a multibyte glyph must be
// translated to block chars before drawing (raw UTF-8 bytes must not be drawn).
TEST(StatusHeader, TranslatesUTF8Name) {
  FakeDisplayDriver d; d.startFrame();
  StatusHeader::draw(d, "Mom\xF0\x9F\x92\x9C", "", 0, true);   // "Mom💜"
  EXPECT_TRUE(d.printed("Mom"));
  EXPECT_FALSE(d.printed("\xF0\x9F\x92\x9C"));                  // raw emoji bytes not drawn
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
