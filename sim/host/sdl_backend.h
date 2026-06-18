// Optional SDL2 graphic-window backend for the display. Renders the 128x64
// framebuffer into a real, freely-resizable window and feeds keyboard input back
// as terminal-style bytes (so the existing key parsing is reused). Compiled only
// when USE_SDL is defined and linked against the vendored sim/vendor/SDL2.framework.
#pragma once
#include <stdint.h>

bool sdlBegin(const char* title, int w, int h, int scale);
void sdlPresent(const uint8_t* fb, int w, int h);   // fb = w*h bytes, 0 = dark, !=0 = lit
void sdlPumpEvents();                                // process window/key events
int  sdlReadByte();                                  // next queued input byte, or -1
bool sdlQuitRequested();                             // window closed / Cmd-Q
void sdlEnd();
