#pragma once
#include <stdint.h>

#define CHILD_Q_MAXOPTS 6
#define CHILD_Q_TEXTLEN 100   // question buffer (incl. null)
#define CHILD_Q_OPTLEN  32    // per-option buffer (incl. null)

struct ChildQuestion {
  char question[CHILD_Q_TEXTLEN];
  char options[CHILD_Q_MAXOPTS][CHILD_Q_OPTLEN];
  int  num_options;
};

// "?<question> | <opt> | <opt> ...", tolerates "<name>: " prefix. trims segments, drops empty
// options, caps at CHILD_Q_MAXOPTS, truncates to buffers. true iff starts with '?' and >= 2 options.
bool parseChildQuestion(const char* text, ChildQuestion& out);
