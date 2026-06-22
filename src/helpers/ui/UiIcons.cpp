#include "UiIcons.h"
#include <stddef.h>

struct IconEntry { const uint8_t* bits; int w, h; };

static const IconEntry TABLE[] = {
  { ICON_BELL_bits, 16, 16 }, { ICON_ENVELOPE_bits, 16, 16 }, { ICON_QUESTION_bits, 16, 16 },
  { ICON_HOME_bits, 16, 16 }, { ICON_PERSON_bits, 16, 16 }, { ICON_GROUP_bits, 16, 16 },
  { ICON_SEND_bits, 16, 16 }, { ICON_GEAR_bits, 16, 16 }, { ICON_LOCK_bits, 16, 16 },
  { ICON_CHECK_bits, 16, 16 }, { ICON_CROSS_bits, 16, 16 }, { ICON_HEART_bits, 16, 16 },
  { ICON_DOT_bits, 8, 8 }, { ICON_CHEVRON_UP_bits, 8, 8 }, { ICON_CHEVRON_DOWN_bits, 8, 8 },
  { ICON_CHEVRON_LEFT_bits, 8, 8 }, { ICON_CHEVRON_RIGHT_bits, 8, 8 },
  { ICON_CHECK_SM_bits, 8, 8 }, { ICON_BTN_bits, 8, 8 },
  { ICON_ENVELOPE_SM_bits, 8, 8 }, { ICON_SEND_SM_bits, 8, 8 }, { ICON_GEAR_SM_bits, 8, 8 },
  { ICON_PERSON_SM_bits, 8, 8 }, { ICON_GROUP_SM_bits, 8, 8 },
};

static_assert(sizeof(TABLE) / sizeof(TABLE[0]) == UI_ICON_COUNT, "icon table size mismatch");

const uint8_t* uiIcon(UiIconId id, int* w, int* h) {
  if (id < 0 || id >= UI_ICON_COUNT) { if (w) *w = 0; if (h) *h = 0; return NULL; }
  const IconEntry& e = TABLE[id];
  if (w) *w = e.w; if (h) *h = e.h;
  return e.bits;
}
