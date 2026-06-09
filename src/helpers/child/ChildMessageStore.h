#pragma once
#include <stdint.h>

#define CHILD_MSG_CAP     20
#define CHILD_MSG_BODY    168   // holds full DM (<=160) or group "name: msg" (~165)
#define CHILD_MSG_ORIGIN  32
#define CHILD_MSG_LABEL   38    // one-line list snippet (incl. null)

struct ChildMessage {
  uint32_t timestamp;
  char     origin[CHILD_MSG_ORIGIN];
  char     text[CHILD_MSG_BODY];
  bool     is_channel;
  bool     read;
};

// Fixed RAM ring of received messages, newest-first via at(0). No heap, no hardware deps.
class ChildMessageStore {
  ChildMessage _slots[CHILD_MSG_CAP];
  char         _labels[CHILD_MSG_CAP][CHILD_MSG_LABEL];
  const char*  _label_ptrs[CHILD_MSG_CAP];
  uint8_t      _count;
  uint8_t      _head;    // physical index of next write slot
  uint8_t      _unread;
public:
  ChildMessageStore();
  void add(const char* origin, const char* text, uint32_t ts, bool is_channel);
  int  count() const  { return _count; }
  int  unread() const { return _unread; }
  void markAllRead();
  const ChildMessage& at(int idx) const;   // idx 0 = newest
  const char* const* labelPtrs();          // rebuilds labels; returns array of count()
};
