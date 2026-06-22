#include <gtest/gtest.h>
#include "helpers/ui/ListMenuScreen.h"
#include "FakeDisplayDriver.h"

struct RecordHandler : public MenuHandler {
  int last_select = -1; int cancels = 0;
  void onMenuSelect(int idx) override { last_select = idx; }
  void onMenuCancel() override { cancels++; }
};

static const char* const ITEMS[] = {"Alpha", "Beta", "Gamma"};

TEST(ListMenuScreen, RendersTitleAndItems) {
  RecordHandler h; ListMenuScreen s; s.set("Menu", ITEMS, 3, &h);
  FakeDisplayDriver d; d.startFrame(); s.render(d);
  EXPECT_TRUE(d.printed("Menu"));
  EXPECT_TRUE(d.printed("Alpha"));
}

// SH1106 renders CP437: an item with a multibyte glyph must be translated to
// block chars (raw UTF-8 bytes must not reach the display).
TEST(ListMenuScreen, TranslatesUTF8Items) {
  static const char* const U[] = { "Hi\xF0\x9F\x98\x80!" };   // "Hi😀!"
  RecordHandler h; ListMenuScreen s; s.set("Menu", U, 1, &h);
  FakeDisplayDriver d; d.startFrame(); s.render(d);
  EXPECT_TRUE(d.printed("Hi"));
  EXPECT_FALSE(d.printed("\xF0\x9F\x98\x80"));
}

// Long items must be clipped to a single line (no wrap), so the one-row
// highlight always matches the drawn text. Fake getTextWidth = 6px/char,
// 128px wide => ~21 chars max per line.
TEST(ListMenuScreen, ClipsLongItemToOneLine) {
  static const char* const L[] = { "0123456789012345678901234567890123456789" }; // 40 chars
  RecordHandler h; ListMenuScreen s; s.set("Menu", L, 1, &h);
  FakeDisplayDriver d; d.startFrame(); s.render(d);
  EXPECT_FALSE(d.printed("0123456789012345678901234567890123456789"));  // not drawn verbatim
  for (auto& t : d.texts) EXPECT_LE((int)t.size(), 21);                 // every drawn line fits
}

TEST(ListMenuScreen, DownThenEnterSelects) {
  RecordHandler h; ListMenuScreen s; s.set("Menu", ITEMS, 3, &h);
  s.handleInput(KEY_DOWN);    // move to Beta
  s.handleInput(KEY_ENTER);
  EXPECT_EQ(h.last_select, 1);
}

TEST(ListMenuScreen, UpWrapsToLast) {
  RecordHandler h; ListMenuScreen s; s.set("Menu", ITEMS, 3, &h);
  s.handleInput(KEY_UP);      // wrap to Gamma
  s.handleInput(KEY_ENTER);
  EXPECT_EQ(h.last_select, 2);
}

TEST(ListMenuScreen, RightSelectsCurrent) {
  RecordHandler h; ListMenuScreen s; s.set("Menu", ITEMS, 3, &h);
  s.handleInput(KEY_RIGHT);   // RIGHT = select (current = Alpha)
  EXPECT_EQ(h.last_select, 0);
}

TEST(ListMenuScreen, LeftCancels) {
  RecordHandler h; ListMenuScreen s; s.set("Menu", ITEMS, 3, &h);
  s.handleInput(KEY_LEFT);    // LEFT = back/cancel
  EXPECT_EQ(h.cancels, 1);
}

TEST(ListMenuScreen, CancelKeyCancels) {
  RecordHandler h; ListMenuScreen s; s.set("Menu", ITEMS, 3, &h);
  s.handleInput(KEY_CANCEL);
  EXPECT_EQ(h.cancels, 1);
}

#include "helpers/ui/UiIcons.h"
static const char* const SIX[] = {"a","b","c","d","e","f"};
TEST(ListMenuScreen, DrawsPerRowIcons) {
  static const int IC[] = { ICON_ENVELOPE_SM, ICON_SEND_SM, ICON_GEAR_SM };
  RecordHandler h; ListMenuScreen s; s.set("Menu", ITEMS, 3, &h, IC);
  FakeDisplayDriver d; d.startFrame(); s.render(d);
  int icons = 0; for (auto& q : d.xbms) if (q[2]==8) icons++;
  EXPECT_GE(icons, 3);
}
TEST(ListMenuScreen, ScrollbarWhenOverflow) {
  RecordHandler h; ListMenuScreen s; s.set("Menu", SIX, 6, &h);   // 6 > 4 visible
  FakeDisplayDriver d; d.startFrame(); s.render(d);
  EXPECT_GE(d.rects, 1);   // scrollbar track is a drawRect
}
TEST(ListMenuScreen, NoScrollbarWhenFits) {
  RecordHandler h; ListMenuScreen s; s.set("Menu", ITEMS, 3, &h);  // 3 <= 4
  FakeDisplayDriver d; d.startFrame(); s.render(d);
  EXPECT_EQ(d.rects, 0);   // no scrollbar
}

TEST(ListMenuScreen, FocusedRowHasCaret) {
  RecordHandler h; ListMenuScreen s; s.set("Menu", ITEMS, 3, &h);   // no per-row icons
  FakeDisplayDriver d; d.startFrame(); s.render(d);
  // selection is an 8x8 caret on exactly the focused row (no full-row inverse fill)
  int carets = 0; for (auto& q : d.xbms) if (q[2]==8) carets++;
  EXPECT_EQ(carets, 1);
  EXPECT_EQ(d.fills, 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
