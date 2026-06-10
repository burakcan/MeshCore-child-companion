// Host buzzer emulation: parses the same RTTTL ringtone strings the firmware
// plays (NonBlockingRtttl) and renders them as square-wave audio on the host.
// On macOS it synthesizes a WAV and plays it via `afplay` (non-blocking).
// Elsewhere it falls back to the terminal bell. Part of sim/, not firmware.
#pragma once

// Play an RTTTL melody, e.g. "MsgRcv3:d=4,o=6,b=200:32e,32g,32b,16c7".
// Non-blocking: returns immediately.
void hostBuzzerPlay(const char* rtttl);

void hostBuzzerToggleMute();   // mirrors the firmware's buzzer-quiet toggle
bool hostBuzzerMuted();
