// Host keyboard -> firmware key codes. Maps the PC keyboard onto the Wio
// Tracker L1's joystick semantics (left/right + press + back) plus a couple
// of simulator-only keys for injecting events. Part of sim/, not firmware.
#pragma once

// Simulator-only sentinels (outside the KEY_* range used by the UI)
enum {
  SIM_KEY_NONE      = 0,
  SIM_KEY_QUIT      = 1,
  SIM_KEY_INJECT_PIN = 2,
};

void hostInputBegin();          // put terminal in raw mode
void hostInputEnd();            // restore terminal

// Non-blocking-ish read with a timeout. Returns:
//   a KEY_* code (see UIScreen.h) to feed to the current screen,
//   a SIM_KEY_* sentinel for harness control,
//   or 0 if nothing was pressed within timeout_ms.
int hostInputPoll(int timeout_ms);
