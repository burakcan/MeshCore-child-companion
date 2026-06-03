#pragma once
#include <string>
#include <vector>
#include <cstring>
#include "helpers/ui/DisplayDriver.h"

class FakeDisplayDriver : public DisplayDriver {
  bool _on = true;
public:
  std::vector<std::string> texts;
  int frames = 0, fills = 0, rects = 0;
  int cursor_x = 0, cursor_y = 0;

  FakeDisplayDriver() : DisplayDriver(128, 64) {}

  bool isOn() override { return _on; }
  void turnOn() override { _on = true; }
  void turnOff() override { _on = false; }
  void clear() override {}
  void startFrame(Color bkg = DARK) override { frames++; texts.clear(); fills = 0; rects = 0; }
  void setTextSize(int sz) override {}
  void setColor(Color c) override {}
  void setCursor(int x, int y) override { cursor_x = x; cursor_y = y; }
  void print(const char* str) override { texts.push_back(str ? str : ""); }
  void fillRect(int x, int y, int w, int h) override { fills++; }
  void drawRect(int x, int y, int w, int h) override { rects++; }
  void drawXbm(int x, int y, const uint8_t* bits, int w, int h) override {}
  uint16_t getTextWidth(const char* str) override { return str ? (uint16_t)(strlen(str) * 6) : 0; }
  void endFrame() override {}

  bool printed(const char* substr) const {
    for (auto& t : texts) if (t.find(substr) != std::string::npos) return true;
    return false;
  }
};
