#include "ChildPresets.h"
#include <string.h>

static const char* const DEFAULTS[] = { "I'm OK", "On my way", "Pick me up", "Yes", "No" };
static const int NUM_DEFAULTS = 5;

ChildPresets::ChildPresets() : _label_count(0) {
  for (int i = 0; i < CHILD_PRESET_SLOTS; i++) _slots[i][0] = 0;
}

void ChildPresets::initDefaults() {
  for (int i = 0; i < CHILD_PRESET_SLOTS; i++) _slots[i][0] = 0;
  for (int i = 0; i < NUM_DEFAULTS; i++) {
    strncpy(_slots[i], DEFAULTS[i], CHILD_PRESET_TEXTLEN - 1);
    _slots[i][CHILD_PRESET_TEXTLEN - 1] = 0;
  }
  labelPtrs();
}

bool ChildPresets::set(int slot, const char* text) {
  if (slot < 0 || slot >= CHILD_PRESET_SLOTS) return false;
  if (text == nullptr) { _slots[slot][0] = 0; return true; }
  strncpy(_slots[slot], text, CHILD_PRESET_TEXTLEN - 1);
  _slots[slot][CHILD_PRESET_TEXTLEN - 1] = 0;
  return true;
}

void ChildPresets::clear(int slot) { set(slot, nullptr); }

bool ChildPresets::isEmpty(int slot) const {
  if (slot < 0 || slot >= CHILD_PRESET_SLOTS) return true;
  return _slots[slot][0] == 0;
}

const char* ChildPresets::text(int slot) const {
  if (slot < 0 || slot >= CHILD_PRESET_SLOTS) return "";
  return _slots[slot];
}

int ChildPresets::packSlot(int slot, uint8_t* buf) const {
  if (slot < 0 || slot >= CHILD_PRESET_SLOTS) return 0;
  int len = (int)strlen(_slots[slot]);
  if (len > CHILD_PRESET_TEXTLEN - 1) len = CHILD_PRESET_TEXTLEN - 1;
  buf[0] = CHILD_PRESET_VERSION;
  buf[1] = (uint8_t)len;
  memcpy(&buf[2], _slots[slot], len);
  return 2 + len;
}

bool ChildPresets::unpackSlot(int slot, const uint8_t* buf, int n) {
  if (slot < 0 || slot >= CHILD_PRESET_SLOTS) return false;
  if (n < 2) return false;
  if (buf[0] != CHILD_PRESET_VERSION) return false;
  int len = buf[1];
  if (len > CHILD_PRESET_TEXTLEN - 1) len = CHILD_PRESET_TEXTLEN - 1;
  if (len > n - 2) len = n - 2;
  memcpy(_slots[slot], &buf[2], len);
  _slots[slot][len] = 0;
  return true;
}

const char* const* ChildPresets::labelPtrs() {
  _label_count = 0;
  for (int i = 0; i < CHILD_PRESET_SLOTS; i++) {
    if (_slots[i][0] != 0) {
      _label_ptrs[_label_count] = _slots[i];
      _label_index[_label_count] = (uint8_t)i;
      _label_count++;
    }
  }
  return _label_ptrs;
}

int ChildPresets::slotForRow(int row) const {
  if (row < 0 || row >= _label_count) return -1;
  return (int)_label_index[row];
}
