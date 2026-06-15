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
  uint8_t  sender[6];      // DM: sender pubkey prefix; zeroed for group
  uint8_t  channel_idx;    // group: channel index; 0xFF for DM
  bool     is_channel;
  bool     read;           // regular messages: read state
  bool     is_question;    // parsed "?q|a|b"
  uint8_t  answer_idx;     // question: 0xFF = unanswered; else chosen option index
};

// still wants attention: unread regular msg, or unanswered question
static inline bool childMsgPending(const ChildMessage& m) {
  return m.is_question ? (m.answer_idx == 0xFF) : !m.read;
}

// from markRead(): identity for the read-ack + whether this was the unread->read flip
struct ChildReadResult {
  bool     transitioned;
  bool     is_channel;
  uint8_t  sender[6];
  uint8_t  channel_idx;
};

// fixed RAM ring of received messages, newest-first via at(0). no heap/hardware deps.
class ChildMessageStore {
  ChildMessage _slots[CHILD_MSG_CAP];
  char         _labels[CHILD_MSG_CAP][CHILD_MSG_LABEL];
  const char*  _label_ptrs[CHILD_MSG_CAP];
  uint8_t      _count;
  uint8_t      _head;    // physical index of next write slot
  int          physIndex(int idx) const;
public:
  ChildMessageStore();
  void add(const char* origin, const char* text, uint32_t ts, bool is_channel,
           const uint8_t* sender_prefix, uint8_t channel_idx, bool is_question);
  int  count() const  { return _count; }
  int  unread() const;                     // pending count (childMsgPending)
  ChildReadResult markRead(int idx);       // mark read; reports transition + identity
  void markAnswered(int idx, uint8_t opt_idx);
  const ChildMessage& at(int idx) const;   // idx 0 = newest
  const char* const* labelPtrs();          // rebuilds labels (? unanswered q, * unread)
};
