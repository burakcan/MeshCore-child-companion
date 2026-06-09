#pragma once
#include <stdint.h>

struct WrapLine {
  uint16_t start;   // byte offset where this line begins
  uint16_t len;     // bytes on this line
};

// greedy word-wrap into lines <= chars_per_line. breaks on spaces, hard-breaks over-long words,
// '\n' forces a break. always >= 1 line; count capped at max_lines.
int childWrapLines(const char* text, int chars_per_line, WrapLine* lines, int max_lines);
