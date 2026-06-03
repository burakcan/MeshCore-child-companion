#pragma once
#include <stdint.h>
#include "UIScreen.h"

class PinHandler {
public:
  virtual void onPinEntered(uint32_t pin) = 0;
  virtual void onPinCancel() {}
};

class PinEntryScreen : public UIScreen {
  static const int NUM_DIGITS = 4;
  const char* _title;
  PinHandler* _handler;
  int _digits[NUM_DIGITS];
  int _pos;
  void resetDigits();
public:
  PinEntryScreen() : _title(""), _handler(nullptr), _pos(0) { resetDigits(); }
  void begin(const char* title, PinHandler* h);
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};
