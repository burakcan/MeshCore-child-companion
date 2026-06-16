#pragma once
#include "ChildPresets.h"   // CHILD_PRESET_SLOTS, CHILD_PRESET_TEXTLEN

// "!preset <n> [text]", n 1-indexed (1..CHILD_PRESET_SLOTS), tolerates "<name>: " prefix.
// empty/whitespace text -> clear. true iff well-formed (caller then suppresses the message).
bool parsePresetCommand(const char* text, int& out_slot1, char* out_text, int out_cap);
