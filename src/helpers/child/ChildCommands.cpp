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

bool parseTzCommand(const char* text, int* out_min) {
  const char* s = skipGroupPrefix(text);
  while (*s == ' ') s++;
  if (strncmp(s, "!tz ", 4) != 0) return false;
  s += 4;
  while (*s == ' ') s++;
  int sign = 1;
  if (*s == '-') { sign = -1; s++; }
  else if (*s == '+') { s++; }
  if (*s < '0' || *s > '9') return false;
  int v = 0, n = 0;
  while (*s >= '0' && *s <= '9' && n < 4) { v = v * 10 + (*s - '0'); s++; n++; }
  v *= sign;
  if (v < -720 || v > 840) return false;     // UTC-12 .. UTC+14
  *out_min = v;
  return true;
}

bool parseNameCommand(const char* text, char* out, int out_size) {
  const char* s = skipGroupPrefix(text);
  while (*s == ' ') s++;
  if (strncmp(s, "!name ", 6) != 0) return false;
  s += 6;
  while (*s == ' ') s++;
  int n = 0;
  while (*s && n < out_size - 1) out[n++] = *s++;
  while (n > 0 && out[n - 1] == ' ') n--;   // trim trailing spaces
  out[n] = 0;
  return n > 0;
}

bool parseRetryCommand(const char* text, bool* out_enabled) {
  const char* s = skipGroupPrefix(text);
  while (*s == ' ') s++;
  if (strncmp(s, "!retry ", 7) != 0) return false;
  s += 7;
  while (*s == ' ') s++;
  if (strncmp(s, "on", 2) == 0 && (s[2] == 0 || s[2] == ' ')) { *out_enabled = true; return true; }
  if (strncmp(s, "off", 3) == 0 && (s[3] == 0 || s[3] == ' ')) { *out_enabled = false; return true; }
  return false;
}

bool childSenderApproved(uint8_t contact_type) {
  return contact_type != 0;   // 0 == ADV_TYPE_NONE
}
