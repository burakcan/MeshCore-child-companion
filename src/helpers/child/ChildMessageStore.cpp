#include "ChildMessageStore.h"
#include <string.h>
#include <stdio.h>

static void copyTrunc(char* dst, const char* src, int dst_size) {
  strncpy(dst, src ? src : "", dst_size - 1);
  dst[dst_size - 1] = 0;
}

ChildMessageStore::ChildMessageStore() : _count(0), _head(0), _unread(0) {
  memset(_slots, 0, sizeof(_slots));
}

void ChildMessageStore::add(const char* origin, const char* text, uint32_t ts, bool is_channel) {
  ChildMessage& m = _slots[_head];
  m.timestamp = ts;
  m.is_channel = is_channel;
  m.read = false;
  copyTrunc(m.origin, origin, CHILD_MSG_ORIGIN);
  copyTrunc(m.text, text, CHILD_MSG_BODY);
  _head = (uint8_t)((_head + 1) % CHILD_MSG_CAP);
  if (_count < CHILD_MSG_CAP) _count++;
  if (_unread < _count) _unread++;
}

void ChildMessageStore::markAllRead() {
  _unread = 0;
  for (int i = 0; i < _count; i++) _slots[i].read = true;
}

const ChildMessage& ChildMessageStore::at(int idx) const {
  // newest is at (_head - 1); idx counts backwards from newest
  int phys = ((int)_head - 1 - idx) % CHILD_MSG_CAP;
  if (phys < 0) phys += CHILD_MSG_CAP;
  return _slots[phys];
}

const char* const* ChildMessageStore::labelPtrs() {
  for (int i = 0; i < _count; i++) {
    const ChildMessage& m = at(i);
    snprintf(_labels[i], CHILD_MSG_LABEL, "(%c) %s: %s",
             m.is_channel ? '#' : 'D', m.origin, m.text);
    _label_ptrs[i] = _labels[i];
  }
  return _label_ptrs;
}
