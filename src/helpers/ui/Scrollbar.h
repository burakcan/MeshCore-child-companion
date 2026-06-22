#pragma once
#include "DisplayDriver.h"

struct ScrollThumb { int y; int h; bool visible; };

namespace Scrollbar {
  // thumb geometry within [track_y, track_y+track_h); visible=false when it all fits
  ScrollThumb thumbRect(int track_y, int track_h, int total, int visible, int offset);
  // 3px track at column x + thumb, only when visible
  void draw(DisplayDriver& d, int x, int track_y, int track_h, int total, int visible, int offset);
}
