#include "ChildPresetCommand.h"
#include <string.h>

// strip leading "<name>: " group prefix (mirrors ChildCommands)
static const char* skipGroupPrefix(const char* s) {
  const char* colon = strstr(s, ": ");
  if (colon != NULL && colon - s < 32) return colon + 2;
  return s;
}

bool parsePresetCommand(const char* text, int& out_slot1, char* out_text, int out_cap) {
  const char* s = skipGroupPrefix(text);
  while (*s == ' ') s++;
  if (strncmp(s, "!preset", 7) != 0) return false;
  s += 7;
  if (*s != ' ') return false;          // require a separator (reject "!presetx")
  while (*s == ' ') s++;
  if (*s < '0' || *s > '9') return false;
  int n = 0, d = 0;
  while (*s >= '0' && *s <= '9' && d < 3) { n = n * 10 + (*s - '0'); s++; d++; }
  if (n < 1 || n > CHILD_PRESET_SLOTS) return false;
  out_slot1 = n;
  while (*s == ' ') s++;
  int i = 0;
  while (s[i] && i < out_cap - 1) { out_text[i] = s[i]; i++; }
  out_text[i] = 0;
  while (i > 0 && out_text[i - 1] == ' ') out_text[--i] = 0;  // trim trailing -> "  " clears
  return true;
}
