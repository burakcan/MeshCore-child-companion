#include "ChildMessageStore.h"
#include <string.h>
#include <stdio.h>

static void copyTrunc(char* dst, const char* src, int dst_size) {
  strncpy(dst, src ? src : "", dst_size - 1);
  dst[dst_size - 1] = 0;
}

ChildMessageStore::ChildMessageStore() : _count(0), _head(0) {
  memset(_slots, 0, sizeof(_slots));
}

int ChildMessageStore::physIndex(int idx) const {
  int phys = ((int)_head - 1 - idx) % CHILD_MSG_CAP;
  if (phys < 0) phys += CHILD_MSG_CAP;
  return phys;
}

void ChildMessageStore::add(const char* origin, const char* text, uint32_t ts, bool is_channel,
                            const uint8_t* sender_prefix, uint8_t channel_idx, bool is_question) {
  ChildMessage& m = _slots[_head];
  m.timestamp = ts;
  m.is_channel = is_channel;
  m.channel_idx = channel_idx;
  m.read = false;
  m.is_question = is_question;
  m.answer_idx = 0xFF;
  if (sender_prefix) memcpy(m.sender, sender_prefix, 6); else memset(m.sender, 0, 6);
  copyTrunc(m.origin, origin, CHILD_MSG_ORIGIN);
  copyTrunc(m.text, text, CHILD_MSG_BODY);
  _head = (uint8_t)((_head + 1) % CHILD_MSG_CAP);
  if (_count < CHILD_MSG_CAP) _count++;
}

int ChildMessageStore::unread() const {
  int n = 0;
  for (int i = 0; i < _count; i++) if (childMsgPending(_slots[i])) n++;
  return n;
}

ChildReadResult ChildMessageStore::markRead(int idx) {
  ChildReadResult r;
  memset(&r, 0, sizeof(r));
  if (idx < 0 || idx >= _count) return r;
  ChildMessage& m = _slots[physIndex(idx)];
  r.transitioned = !m.read;
  r.is_channel = m.is_channel;
  memcpy(r.sender, m.sender, 6);
  r.channel_idx = m.channel_idx;
  m.read = true;
  return r;
}

void ChildMessageStore::markAnswered(int idx, uint8_t opt_idx) {
  if (idx < 0 || idx >= _count) return;
  _slots[physIndex(idx)].answer_idx = opt_idx;
}

const ChildMessage& ChildMessageStore::at(int idx) const {
  return _slots[physIndex(idx)];
}

const char* const* ChildMessageStore::labelPtrs() {
  for (int i = 0; i < _count; i++) {
    const ChildMessage& m = at(i);
    const char* mark = "";
    if (m.is_question) { if (m.answer_idx == 0xFF) mark = "? "; }
    else if (!m.read)  { mark = "* "; }
    // group/DM shown by per-row icon; marker is just unread/question
    snprintf(_labels[i], CHILD_MSG_LABEL, "%s%s: %s", mark, m.origin, m.text);
    _label_ptrs[i] = _labels[i];
  }
  return _label_ptrs;
}
