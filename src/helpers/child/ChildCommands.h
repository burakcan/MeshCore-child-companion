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

// "!tz <minutes>" signed UTC offset; out-of-range (valid -720..+840) -> false
bool parseTzCommand(const char* text, int* out_min);

// "!name <newname>" -> trimmed name in out; false if empty
bool parseNameCommand(const char* text, char* out, int out_size);

// Approved sender == any real contact. Anon/transient contacts use ADV_TYPE_NONE (0).
// Takes the raw ContactInfo.type so this stays host-portable (no firmware headers).
bool childSenderApproved(uint8_t contact_type);
