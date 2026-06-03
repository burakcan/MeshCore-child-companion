#pragma once
#include <stdint.h>

enum ChildCmd { CHILD_CMD_NONE = 0, CHILD_CMD_PIN_CHANGE = 1 };

struct PinChange {
  uint32_t old_pin;
  uint32_t new_pin;
};

// all parsers tolerate a leading "<name>: " group prefix.

// "!pin <old> <new>"
ChildCmd parseChildCommand(const char* text, PinChange& out);
