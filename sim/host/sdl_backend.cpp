#include "sdl_backend.h"
#include <SDL2/SDL.h>
#include <deque>
#include <cstdio>

static SDL_Window*   s_win = nullptr;
static SDL_Renderer* s_ren = nullptr;
static SDL_Texture*  s_tex = nullptr;
static int s_w = 0, s_h = 0;
static bool s_quit = false;
static std::deque<int> s_keys;

static void push(int b) { s_keys.push_back(b); }
static void pushArrow(char dir) { push(0x1b); push('['); push(dir); }   // ESC [ A/B/C/D

bool sdlBegin(const char* title, int w, int h, int scale) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) { fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError()); return false; }
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");   // nearest-neighbour (crisp pixels)
  s_win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           w * scale, h * scale, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  if (!s_win) { fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError()); return false; }
  s_ren = SDL_CreateRenderer(s_win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!s_ren) s_ren = SDL_CreateRenderer(s_win, -1, 0);
  SDL_RenderSetLogicalSize(s_ren, w, h);             // scale the 128x64 image to fill, keep aspect
  s_tex = SDL_CreateTexture(s_ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
  s_w = w; s_h = h;
  return true;
}

void sdlPresent(const uint8_t* fb, int w, int h) {
  if (!s_tex) return;
  void* pixels; int pitch;
  if (SDL_LockTexture(s_tex, nullptr, &pixels, &pitch) != 0) return;
  for (int y = 0; y < h; y++) {
    uint32_t* row = (uint32_t*)((uint8_t*)pixels + y * pitch);
    for (int x = 0; x < w; x++)
      row[x] = fb[y * w + x] ? 0xFFE8E8E8u : 0xFF101018u;   // lit = light grey, off = near-black
  }
  SDL_UnlockTexture(s_tex);
  SDL_RenderClear(s_ren);
  SDL_RenderCopy(s_ren, s_tex, nullptr, nullptr);
  SDL_RenderPresent(s_ren);
}

void sdlPumpEvents() {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT) { s_quit = true; continue; }
    if (e.type != SDL_KEYDOWN) continue;
    switch (e.key.keysym.sym) {
      case SDLK_RETURN: case SDLK_KP_ENTER: push('\r'); break;
      case SDLK_LEFT:  pushArrow('D'); break;   // ESC [ D
      case SDLK_RIGHT: pushArrow('C'); break;
      case SDLK_UP:    pushArrow('A'); break;
      case SDLK_DOWN:  pushArrow('B'); break;
      case SDLK_a: push('a'); break;
      case SDLK_d: push('d'); break;
      case SDLK_b: push('b'); break;
      case SDLK_l: push('l'); break;
      case SDLK_q: push('q'); break;
      case SDLK_ESCAPE: s_quit = true; break;
      default: break;
    }
  }
}

int  sdlReadByte() { if (s_keys.empty()) return -1; int b = s_keys.front(); s_keys.pop_front(); return b; }
bool sdlQuitRequested() { return s_quit; }

void sdlEnd() {
  if (s_tex) SDL_DestroyTexture(s_tex);
  if (s_ren) SDL_DestroyRenderer(s_ren);
  if (s_win) SDL_DestroyWindow(s_win);
  SDL_Quit();
}
