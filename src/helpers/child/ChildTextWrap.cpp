#include "ChildTextWrap.h"
#include <string.h>

int childWrapLines(const char* text, int chars_per_line, WrapLine* lines, int max_lines) {
  if (chars_per_line < 1) chars_per_line = 1;
  int n = 0;
  int len = text ? (int)strlen(text) : 0;
  int i = 0;
  while (n < max_lines) {
    int line_start = i;
    int last_break = -1;           // last space on this line
    int col = 0;
    while (i < len && text[i] != '\n' && col < chars_per_line) {
      if (text[i] == ' ') last_break = i;
      i++; col++;
    }
    int line_end;                  // exclusive
    if (i >= len || text[i] == '\n') {
      line_end = i;
    } else if (last_break > line_start) {
      line_end = last_break;       // wrap at last space
      i = last_break + 1;
    } else {
      line_end = i;                // hard break mid-word
    }
    lines[n].start = (uint16_t)line_start;
    lines[n].len = (uint16_t)(line_end - line_start);
    n++;
    if (i < len && text[i] == '\n') i++;   // consume newline
    if (i >= len) break;
  }
  if (n == 0) { lines[0].start = 0; lines[0].len = 0; n = 1; }   // empty text
  return n;
}
