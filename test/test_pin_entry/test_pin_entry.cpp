#include <gtest/gtest.h>
#include "helpers/ui/PinEntryScreen.h"
#include "FakeDisplayDriver.h"

struct PinRecorder : public PinHandler {
  uint32_t got = 0xFFFFFFFF; int cancels = 0;
  void onPinEntered(uint32_t pin) override { got = pin; }
  void onPinCancel() override { cancels++; }
};

// Helper: set the active digit to `d` by incrementing from 0.
static void setDigit(PinEntryScreen& s, int d) {
  for (int i = 0; i < d; i++) s.handleInput(KEY_RIGHT);
  s.handleInput(KEY_ENTER);
}

TEST(PinEntryScreen, EntersFourDigitPin) {
  PinRecorder r; PinEntryScreen s; s.begin("Enter PIN", &r);
  setDigit(s, 1); setDigit(s, 2); setDigit(s, 3); setDigit(s, 4);
  EXPECT_EQ(r.got, 1234u);
}

TEST(PinEntryScreen, LeftWrapsDigitDownTo9) {
  PinRecorder r; PinEntryScreen s; s.begin("Enter PIN", &r);
  s.handleInput(KEY_LEFT);   // 0 -> 9
  s.handleInput(KEY_ENTER);
  setDigit(s, 0); setDigit(s, 0); setDigit(s, 0);
  EXPECT_EQ(r.got, 9000u);
}

TEST(PinEntryScreen, CancelInvokesHandler) {
  PinRecorder r; PinEntryScreen s; s.begin("Enter PIN", &r);
  s.handleInput(KEY_CANCEL);
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
