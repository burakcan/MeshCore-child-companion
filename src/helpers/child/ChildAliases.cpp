#include "ChildAliases.h"
#include <string.h>

static void copyName(char* dst, const char* src) {
  strncpy(dst, src ? src : "", CHILD_ALIAS_NAMELEN - 1);
  dst[CHILD_ALIAS_NAMELEN - 1] = 0;
}

void ChildAliases::set(const char* original, const char* display) {
  if (!original || !*original || !display || !*display) return;
  for (int i = 0; i < _count; i++)            // re-rename: original is a current display name
    if (strcmp(_to[i], original) == 0) { copyName(_to[i], display); return; }
  for (int i = 0; i < _count; i++)            // update existing key
    if (strcmp(_from[i], original) == 0) { copyName(_to[i], display); return; }
  int slot = (_count < CHILD_ALIAS_MAX) ? _count++ : 0;   // full: overwrite oldest
  copyName(_from[slot], original);
  copyName(_to[slot], display);
}

const char* ChildAliases::lookup(const char* name) const {
  if (!name) return 0;
  for (int i = 0; i < _count; i++)
    if (strcmp(_from[i], name) == 0) return _to[i];
  return 0;
}

int ChildAliases::pack(uint8_t* buf) const {
  memset(buf, 0, CHILD_ALIAS_SAVE_LEN);
  buf[0] = (uint8_t)_count;
  int off = 1;
  for (int i = 0; i < _count && i < CHILD_ALIAS_MAX; i++) {
    memcpy(buf + off, _from[i], CHILD_ALIAS_NAMELEN); off += CHILD_ALIAS_NAMELEN;
    memcpy(buf + off, _to[i], CHILD_ALIAS_NAMELEN);   off += CHILD_ALIAS_NAMELEN;
  }
  return CHILD_ALIAS_SAVE_LEN;
}

bool ChildAliases::unpack(const uint8_t* buf, int len) {
  if (len < 1) return false;
  int c = buf[0];
  if (c > CHILD_ALIAS_MAX) c = CHILD_ALIAS_MAX;
  if (len < 1 + c * CHILD_ALIAS_NAMELEN * 2) return false;
  _count = c;
  int off = 1;
  for (int i = 0; i < c; i++) {
    copyName(_from[i], (const char*)(buf + off)); off += CHILD_ALIAS_NAMELEN;
    copyName(_to[i], (const char*)(buf + off));   off += CHILD_ALIAS_NAMELEN;
  }
  return true;
}
