#pragma once
#include <stdint.h>

#define CHILD_ALIAS_MAX      4
#define CHILD_ALIAS_NAMELEN  18   // incl null
#define CHILD_ALIAS_SAVE_LEN (1 + CHILD_ALIAS_MAX * (CHILD_ALIAS_NAMELEN * 2))   // 145

// on-air name -> display name. group msgs carry no pubkey, only the embedded "<name>: ..."
// string, so the match is string->string. DMs rename the contact directly; this is what makes
// "!name" reach group messages too.
class ChildAliases {
  char _from[CHILD_ALIAS_MAX][CHILD_ALIAS_NAMELEN];
  char _to[CHILD_ALIAS_MAX][CHILD_ALIAS_NAMELEN];
  int  _count;
public:
  ChildAliases() : _count(0) {}
  void clear() { _count = 0; }
  // original matching an existing original OR display updates that entry (re-rename keeps one
  // entry); else add new, overwriting oldest when full.
  void set(const char* original, const char* display);
  const char* lookup(const char* name) const;   // by original; nullptr if none
  int count() const { return _count; }
  int  pack(uint8_t* buf) const;            // writes CHILD_ALIAS_SAVE_LEN bytes
  bool unpack(const uint8_t* buf, int len);
};
