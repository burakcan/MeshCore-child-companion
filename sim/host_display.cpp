#include "host_display.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>

// Vendored copy of Adafruit_GFX's classic 5x7 font ("font[]"), so on-screen
// text matches the device pixel-for-pixel (advance = 6px * size).
#include "glcdfont.h"

static const int W = 128;
static const int H = 64;

HostDisplay::HostDisplay()
  : DisplayDriver(W, H), _fb(W * H, 0), _cx(0), _cy(0),
    _tsize(1), _color(1), _on(true), _status(nullptr) {}

void HostDisplay::setPixel(int x, int y, uint8_t v) {
  if (x < 0 || x >= W || y < 0 || y >= H) return;
  _fb[y * W + x] = v;
}

void HostDisplay::clear() {
  std::fill(_fb.begin(), _fb.end(), 0);
}

void HostDisplay::startFrame(Color bkg) {
  uint8_t v = (bkg == DARK) ? 0 : 1;
  std::fill(_fb.begin(), _fb.end(), v);
  _cx = _cy = 0;
  _tsize = 1;
  _color = 1;
}

void HostDisplay::setTextSize(int sz) { _tsize = sz < 1 ? 1 : sz; }

void HostDisplay::setColor(Color c) { _color = (c == DARK) ? 0 : 1; }

void HostDisplay::setCursor(int x, int y) { _cx = x; _cy = y; }

// Classic Adafruit drawChar: 5 columns of bitmap + 1 spacing column, 7 rows.
void HostDisplay::drawCharAt(int x, int y, unsigned char ch) {
  for (int col = 0; col < 6; col++) {
    uint8_t bits = (col < 5) ? font[ch * 5 + col] : 0x00;
    for (int row = 0; row < 8; row++) {
      if (bits & 0x01) {
        // scale by text size
        for (int dx = 0; dx < _tsize; dx++)
          for (int dy = 0; dy < _tsize; dy++)
            setPixel(x + col * _tsize + dx, y + row * _tsize + dy, _color);
      }
      bits >>= 1;
    }
  }
}

void HostDisplay::print(const char* str) {
  if (!str) return;
  for (const char* p = str; *p; p++) {
    if (*p == '\n') { _cx = 0; _cy += 8 * _tsize; continue; }
    if (*p == '\r') continue;
    drawCharAt(_cx, _cy, (unsigned char)*p);
    _cx += 6 * _tsize;
  }
}

uint16_t HostDisplay::getTextWidth(const char* str) {
  return (uint16_t)(std::strlen(str ? str : "") * 6 * _tsize);
}

void HostDisplay::fillRect(int x, int y, int w, int h) {
  for (int yy = y; yy < y + h; yy++)
    for (int xx = x; xx < x + w; xx++)
      setPixel(xx, yy, _color);
}

void HostDisplay::drawRect(int x, int y, int w, int h) {
  for (int xx = x; xx < x + w; xx++) { setPixel(xx, y, _color); setPixel(xx, y + h - 1, _color); }
  for (int yy = y; yy < y + h; yy++) { setPixel(x, yy, _color); setPixel(x + w - 1, yy, _color); }
}

// XBM: LSB-first within each byte, rows padded to whole bytes.
void HostDisplay::drawXbm(int x, int y, const uint8_t* bits, int w, int h) {
  int row_bytes = (w + 7) / 8;
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      uint8_t byte = bits[j * row_bytes + (i / 8)];
      if (byte & (1 << (i & 7))) setPixel(x + i, y + j, _color);
    }
  }
}

// Render the framebuffer to the terminal. Each character cell covers two
// vertical pixels using Unicode half-block glyphs, so 128x64 -> 128x32 chars.
void HostDisplay::endFrame() {
  std::string out;
  out.reserve(W * (H / 2) * 4 + 256);
  out += "\x1b[H";                     // cursor home (overdraw in place)
  out += "+";                          // top border
  for (int x = 0; x < W; x++) out += "-";
  out += "+\n";
  for (int y = 0; y < H; y += 2) {
    out += "|";
    for (int x = 0; x < W; x++) {
      bool top = _fb[y * W + x] != 0;
      bool bot = (y + 1 < H) && _fb[(y + 1) * W + x] != 0;
      if (top && bot)      out += "█"; // full block
      else if (top)        out += "▀"; // upper half
      else if (bot)        out += "▄"; // lower half
      else                 out += " ";
    }
    out += "|\n";
  }
  out += "+";
  for (int x = 0; x < W; x++) out += "-";
  out += "+\n";
  if (_status) { out += _status; out += "\x1b[K\n"; }
  fputs(out.c_str(), stdout);
  fflush(stdout);
}
