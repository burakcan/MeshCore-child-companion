#include "Scrollbar.h"

ScrollThumb Scrollbar::thumbRect(int track_y, int track_h, int total, int visible, int offset) {
  ScrollThumb t = { track_y, track_h, false };
  if (total <= visible || total <= 0 || track_h <= 0) return t;   // fits, no bar
  t.visible = true;
  int h = (int)((long)track_h * visible / total);
  if (h < 4) h = 4;                                  // min thumb
  if (h > track_h) h = track_h;
  int max_off = total - visible;
  if (offset < 0) offset = 0;
  if (offset > max_off) offset = max_off;
  int y = track_y + (int)((long)(track_h - h) * offset / max_off);
  t.y = y; t.h = h;
  return t;
}

void Scrollbar::draw(DisplayDriver& d, int x, int track_y, int track_h, int total, int visible, int offset) {
  ScrollThumb t = thumbRect(track_y, track_h, total, visible, offset);
  if (!t.visible) return;
  d.setColor(DisplayDriver::LIGHT);
  d.drawRect(x, track_y, 3, track_h);   // track
  d.fillRect(x, t.y, 3, t.h);           // thumb
}
