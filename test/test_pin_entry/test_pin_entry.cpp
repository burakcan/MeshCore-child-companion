#include <gtest/gtest.h>
#include "helpers/ui/PinEntryScreen.h"
#include "FakeDisplayDriver.h"

struct PinRecorder : public PinHandler {
  uint32_t got = 0xFFFFFFFF; int cancels = 0;
  void onPinEntered(uint32_t pin) override { got = pin; }
  void onPinCancel() override { cancels++; }
};

// Helper: set the active digit to `d` (up = +1) and advance with ENTER.
static void setDigit(PinEntryScreen& s, int d) {
  for (int i = 0; i < d; i++) s.handleInput(KEY_UP);
  s.handleInput(KEY_ENTER);
}

TEST(PinEntryScreen, EntersFourDigitPin) {
  PinRecorder r; PinEntryScreen s; s.begin("Enter PIN", &r);
  setDigit(s, 1); setDigit(s, 2); setDigit(s, 3); setDigit(s, 4);
  EXPECT_EQ(r.got, 1234u);
}

TEST(PinEntryScreen, DownWrapsDigitTo9) {
  PinRecorder r; PinEntryScreen s; s.begin("Enter PIN", &r);
  s.handleInput(KEY_DOWN);   // 0 -> 9 (down = decrement)
  s.handleInput(KEY_ENTER);
  setDigit(s, 0); setDigit(s, 0); setDigit(s, 0);
  EXPECT_EQ(r.got, 9000u);
}

TEST(PinEntryScreen, RightAdvancesLikeEnter) {
  PinRecorder r; PinEntryScreen s; s.begin("Enter PIN", &r);
  for (int i = 0; i < 5; i++) s.handleInput(KEY_UP);   // digit 0 = 5
  s.handleInput(KEY_RIGHT);                            // advance (RIGHT = advance)
  setDigit(s, 0); setDigit(s, 0); setDigit(s, 0);
  EXPECT_EQ(r.got, 5000u);
}

TEST(PinEntryScreen, CancelInvokesHandler) {
  PinRecorder r; PinEntryScreen s; s.begin("Enter PIN", &r);
  s.handleInput(KEY_CANCEL);
  EXPECT_EQ(r.cancels, 1);
}

TEST(PinEntryScreen, LeftCancels) {
  PinRecorder r; PinEntryScreen s; s.begin("Enter PIN", &r);
  s.handleInput(KEY_LEFT);   // LEFT = cancel
  EXPECT_EQ(r.cancels, 1);
}

TEST(PinEntryScreen, RendersTitle) {
  PinRecorder r; PinEntryScreen s; s.begin("Enter PIN", &r);
  FakeDisplayDriver d; d.startFrame(); s.render(d);
  EXPECT_TRUE(d.printed("Enter PIN"));
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
