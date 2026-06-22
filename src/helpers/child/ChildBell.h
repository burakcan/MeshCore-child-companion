#pragma once

// true iff text is exactly the bell emoji (UTF-8 F0 9F 94 94), spaces trimmed, optional
// "<name>: " group prefix. mid-sentence or any trailing content -> false. NULL/empty -> false.
bool parseChildBell(const char* text);
