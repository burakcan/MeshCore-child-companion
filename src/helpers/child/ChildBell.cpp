#include "ChildBell.h"
#include <string.h>

static bool isBell(const char* s) {
  return s[0] == (char)0xF0 && s[1] == (char)0x9F &&
         s[2] == (char)0x94 && s[3] == (char)0x94;
}

bool parseChildBell(const char* text) {
  if (text == NULL) return false;

  const char* q = text;
  while (*q == ' ') q++;
  if (!isBell(q)) {                    // not bare -> try after "<name>: " prefix
    const char* c = strstr(text, ": ");
    if (c == NULL) return false;
    q = c + 2;
    while (*q == ' ') q++;
    if (!isBell(q)) return false;
  }
  q += 4;                             // past the emoji
  while (*q == ' ') q++;
  return *q == 0;                     // nothing else allowed
}
