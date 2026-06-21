// Optional SDL2 graphic-window backend for the display. Renders the 128x64
// framebuffer into a real, freely-resizable window and reports key DOWN/UP
// events (so a real key-hold becomes a button long-press, like the hardware).
// Compiled only when USE_SDL is defined; linked against sim/vendor/SDL2.framework.
#pragma once
#include <stdint.h>

// Normalized button ids (the backend hides SDL keycodes).
enum {
  SK_NONE = 0, SK_PRESS, SK_LEFT, SK_RIGHT, SK_UP, SK_DOWN, SK_BACK, SK_QUIT
};

bool sdlBegin(const char* title, int w, int h, int scale);
void sdlPresent(const uint8_t* fb, int w, int h);   // fb = w*h bytes, 0 = dark, !=0 = lit
void sdlPumpEvents();                                // process window/key events
bool sdlNextKey(int* id, bool* down);                // pop one key event; false if none
bool sdlQuitRequested();                             // window closed / Cmd-Q
void sdlEnd();
