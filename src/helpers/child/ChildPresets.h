#pragma once
#include <stdint.h>

#define CHILD_PRESET_SLOTS    10
#define CHILD_PRESET_TEXTLEN  81     // 80 chars + null
#define CHILD_PRESET_VERSION   1

// fixed table of canned messages, one per slot. pure logic, no heap/hardware deps.
// slot args 0-indexed (0..CHILD_PRESET_SLOTS-1); empty slot is text[0]==0.
class ChildPresets {
  char        _slots[CHILD_PRESET_SLOTS][CHILD_PRESET_TEXTLEN];
  const char* _label_ptrs[CHILD_PRESET_SLOTS];   // non-empty slots, for pick-list
  uint8_t     _label_index[CHILD_PRESET_SLOTS];   // pick-list row -> slot
  int         _label_count;
public:
  ChildPresets();
  void initDefaults();                            // seed slots 0..4, clear 5..9
  bool set(int slot, const char* text);           // nullptr/"" clears; false out of range
  void clear(int slot);
  bool isEmpty(int slot) const;
  const char* text(int slot) const;               // "" if out of range or empty
  // per-slot blob: [version:1][len:1][text:len]. caller zero-pads to DataStore min.
  int  packSlot(int slot, uint8_t* buf) const;    // bytes written (2+len), 0 if out of range
  bool unpackSlot(int slot, const uint8_t* buf, int n);  // len==0 -> empty
  const char* const* labelPtrs();                 // rebuilds label arrays
  int  labelCount() const { return _label_count; }
  int  slotForRow(int row) const;                 // -1 if out of range
};
