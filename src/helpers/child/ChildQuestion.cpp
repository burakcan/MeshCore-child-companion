#include "ChildQuestion.h"
#include <string.h>

// copy [start,end) into dst, spaces trimmed, bounded + null-terminated
static void copyTrimmed(char* dst, int dst_size, const char* start, const char* end) {
  while (start < end && *start == ' ') start++;
  while (end > start && *(end - 1) == ' ') end--;
  int n = (int)(end - start);
  if (n > dst_size - 1) n = dst_size - 1;
  memcpy(dst, start, n);
  dst[n] = 0;
}

bool parseChildQuestion(const char* text, ChildQuestion& out) {
  if (text == NULL) return false;

  const char* p = text;
  if (*p != '?') {
    const char* c = strstr(text, ": ");   // tolerate "<name>: " group prefix
    if (c == NULL || c[2] != '?') return false;
    p = c + 2;
  }
  p++;   // skip '?'

  out.num_options = 0;
  out.question[0] = 0;
  const char* seg = p;
  int seg_idx = 0;
  for (const char* cur = p; ; cur++) {
    if (*cur == '|' || *cur == 0) {
      if (seg_idx == 0) {
        copyTrimmed(out.question, CHILD_Q_TEXTLEN, seg, cur);
      } else {
        char tmp[CHILD_Q_OPTLEN];
        copyTrimmed(tmp, CHILD_Q_OPTLEN, seg, cur);
        if (tmp[0] != 0 && out.num_options < CHILD_Q_MAXOPTS) {
          memcpy(out.options[out.num_options++], tmp, CHILD_Q_OPTLEN);
        }
      }
      seg_idx++;
      if (*cur == 0) break;
      seg = cur + 1;
    }
  }
  return out.question[0] != 0 && out.num_options >= 2;
}
