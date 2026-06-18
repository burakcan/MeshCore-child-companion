#include "host_display.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <sys/ioctl.h>
#include <unistd.h>

// Terminal resize handling: a SIGWINCH marks that we must fully clear before the
// next frame (otherwise the in-place overdraw misaligns and garbles).
static volatile sig_atomic_t g_winch = 1;   // start true -> clear on first frame
static void onWinch(int) { g_winch = 1; }
static bool g_winch_installed = false;

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

// The device's DisplayDriver::drawXbm maps to Adafruit_GFX drawBitmap, which is
// MSB-first (bit 0x80 = leftmost pixel), NOT XBM's LSB-first. Match that or the
// logo/icons come out byte-mirrored (garbled). Rows padded to whole bytes.
void HostDisplay::drawXbm(int x, int y, const uint8_t* bits, int w, int h) {
  int row_bytes = (w + 7) / 8;
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      uint8_t byte = bits[j * row_bytes + (i / 8)];
      if (byte & (0x80 >> (i & 7))) setPixel(x + i, y + j, _color);  // MSB-first
    }
  }
}

// Render the framebuffer to the terminal. Each character cell covers two
// vertical pixels using Unicode half-block glyphs, so 128x64 -> 128x32 chars.
// 2x2-pixel quadrant glyphs, indexed by bits: TL|TR<<1|BL<<2|BR<<3.
static const char* QUAD[16] = {
  " ", "▘", "▝", "▀", "▖", "▌", "▞", "▛",
  "▗", "▚", "▐", "▜", "▄", "▙", "▟", "█"
};

void HostDisplay::endFrame() {
  if (_silent) return;   // SDL mode presents the framebuffer directly; no terminal output
  if (!g_winch_installed) { signal(SIGWINCH, onWinch); g_winch_installed = true; }

  struct winsize ws{};
  int cols = (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) ? ws.ws_col : 200;

  // SIM_DISPLAY=half|quad forces a renderer; default auto-fits to the pane width.
  static int forced = -2;   // -2 uninit, -1 auto, 1 half, 2 quad
  if (forced == -2) {
    const char* m = getenv("SIM_DISPLAY");
    forced = (m && !strcmp(m, "half")) ? 1 : (m && !strcmp(m, "quad")) ? 2 : -1;
  }

  // Pick the densest renderer that fits: 1 char/px (best aspect, needs W+2 cols),
  // else 2 px/char quadrants (half the width), else a message.
  int cw;                       // chars used per pixel-column (1 = half-block, 2px wide cell = quadrant)
  if (forced == 2)            cw = (cols >= W / 2 + 2) ? 2 : 0;
  else if (forced == 1)       cw = (cols >= W + 2) ? 1 : 0;
  else if (cols >= W + 2)      cw = 1;
  else if (cols >= W / 2 + 2) cw = 2;
  else                   cw = 0;
  int fw = (cw == 1) ? W : W / 2;   // frame inner width in chars

  std::string out;
  out.reserve(fw * (H / 2) * 4 + 256);
  if (g_winch) { out += "\x1b[2J"; g_winch = 0; }   // resize -> full clear once
  out += "\x1b[H";

  if (cw == 0) {
    char msg[160];
    snprintf(msg, sizeof(msg), "[ pane too narrow: need >= %d cols, have %d — widen it ]", W / 2 + 2, cols);
    out += msg; out += "\x1b[K\n\x1b[J";
    fputs(out.c_str(), stdout); fflush(stdout);
    return;
  }

  out += "+";
  for (int x = 0; x < fw; x++) out += "-";
  out += "+\x1b[K\n";
  for (int y = 0; y < H; y += 2) {
    out += "|";
    if (cw == 1) {                                  // half-block: 1 px wide per char
      for (int x = 0; x < W; x++) {
        bool top = _fb[y * W + x] != 0;
        bool bot = (y + 1 < H) && _fb[(y + 1) * W + x] != 0;
        out += (top && bot) ? "█" : top ? "▀" : bot ? "▄" : " ";
      }
    } else {                                        // quadrant: 2x2 px per char
      for (int x = 0; x < W; x += 2) {
        int idx = (_fb[y * W + x] != 0)
                | ((_fb[y * W + x + 1] != 0) << 1)
                | ((_fb[(y + 1) * W + x] != 0) << 2)
                | ((_fb[(y + 1) * W + x + 1] != 0) << 3);
        out += QUAD[idx];
      }
    }
    out += "|\x1b[K\n";
  }
  out += "+";
  for (int x = 0; x < fw; x++) out += "-";
  out += "+\x1b[K\n";
  if (_status) {
    std::string s(_status);
    if ((int)s.size() > cols) s.resize(cols);        // don't let the help line wrap
    out += s; out += "\x1b[K\n";
  }
  out += "\x1b[J";
  fputs(out.c_str(), stdout);
  fflush(stdout);
}
