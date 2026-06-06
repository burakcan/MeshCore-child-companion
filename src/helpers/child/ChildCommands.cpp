#include "ChildCommands.h"
#include <string.h>

// strip a leading "<name>: " group prefix
static const char* skipGroupPrefix(const char* s) {
  const char* colon = strstr(s, ": ");
  if (colon != NULL && colon - s < 32) return colon + 2;
  return s;
}

// digits -> *val, advance past digits + trailing spaces; false if no digit
static bool parseUint(const char** pp, uint32_t* val) {
  const char* p = *pp;
  if (*p < '0' || *p > '9') return false;
  uint32_t v = 0; int n = 0;
  while (*p >= '0' && *p <= '9' && n < 9) { v = v * 10 + (*p - '0'); p++; n++; }
  while (*p == ' ') p++;
  *pp = p; *val = v;
  return true;
}

ChildCmd parseChildCommand(const char* text, PinChange& out) {
  const char* s = skipGroupPrefix(text);
  while (*s == ' ') s++;
  if (strncmp(s, "!pin ", 5) != 0) return CHILD_CMD_NONE;
  s += 5;
  while (*s == ' ') s++;
  if (!parseUint(&s, &out.old_pin)) return CHILD_CMD_NONE;
  if (!parseUint(&s, &out.new_pin)) return CHILD_CMD_NONE;
  return CHILD_CMD_PIN_CHANGE;
}

bool childSenderApproved(uint8_t contact_type) {
  return contact_type != 0;   // 0 == ADV_TYPE_NONE
}
