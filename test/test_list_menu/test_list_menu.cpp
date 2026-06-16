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

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
