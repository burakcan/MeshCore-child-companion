// Host-side DisplayDriver: renders the firmware UI into a 128x64 1-bit
// framebuffer and prints it to the terminal using Unicode half-blocks.
// Part of the PC simulator harness (sim/), NOT firmware. The real UI screen
// classes draw through DisplayDriver exactly as they do on the device.
#pragma once

#include <helpers/ui/DisplayDriver.h>
#include <stdint.h>
#include <vector>

class HostDisplay : public DisplayDriver {
  std::vector<uint8_t> _fb;   // width*height, 0 = dark, 1 = lit
  int _cx, _cy;               // text cursor (top-left of next glyph)
  int _tsize;                // text size multiplier
  uint8_t _color;            // 1 = lit, 0 = dark
  bool _on;
  bool _silent = false;      // when true, endFrame() does nothing (SDL presents instead)
  const char* _status;       // optional status line under the frame

  void setPixel(int x, int y, uint8_t v);
  void drawCharAt(int x, int y, unsigned char ch);

public:
  HostDisplay();

  void setStatusLine(const char* s) { _status = s; }
  void setSilent(bool s) { _silent = s; }            // SDL mode: skip terminal output
  const uint8_t* framebuffer() const { return _fb.data(); }   // for the SDL backend

  // DisplayDriver interface
  bool isOn() override { return _on; }
  void turnOn() override { _on = true; }
  void turnOff() override { _on = false; }
  void clear() override;
  void startFrame(Color bkg = DARK) override;
  void setTextSize(int sz) override;
  void setColor(Color c) override;
  void setCursor(int x, int y) override;
  void print(const char* str) override;
  void fillRect(int x, int y, int w, int h) override;
  void drawRect(int x, int y, int w, int h) override;
  void drawXbm(int x, int y, const uint8_t* bits, int w, int h) override;
  uint16_t getTextWidth(const char* str) override;
  void endFrame() override;   // renders framebuffer to the terminal
};
