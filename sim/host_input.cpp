#include "host_input.h"
#include <helpers/ui/UIScreen.h>   // KEY_* codes
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <cstdio>

static struct termios s_orig;

void hostInputBegin() {
  tcgetattr(STDIN_FILENO, &s_orig);
  struct termios raw = s_orig;
  raw.c_lflag &= ~(ICANON | ECHO);  // no line buffering, no echo
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void hostInputEnd() {
  tcsetattr(STDIN_FILENO, TCSANOW, &s_orig);
}

static int readByteTimeout(int timeout_ms) {
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);
  struct timeval tv;
  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (timeout_ms % 1000) * 1000;
  int r = select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv);
  if (r <= 0) return -1;
  unsigned char c;
  if (read(STDIN_FILENO, &c, 1) != 1) return -1;
  return c;
}

int hostInputPoll(int timeout_ms) {
  int c = readByteTimeout(timeout_ms);
  if (c < 0) return 0;

  // Arrow keys arrive as ESC [ A/B/C/D. A lone ESC = back/cancel.
  if (c == 0x1b) {
    int c2 = readByteTimeout(5);
    if (c2 < 0) return KEY_CANCEL;       // bare ESC -> back
    if (c2 == '[') {
      int c3 = readByteTimeout(5);
      switch (c3) {
        case 'A': return KEY_UP;
        case 'B': return KEY_DOWN;
        case 'C': return KEY_RIGHT;
        case 'D': return KEY_LEFT;
      }
    }
    return KEY_CANCEL;
  }

  switch (c) {
    case '\r': case '\n': return KEY_ENTER;   // joystick press
    case 0x7f: case '\b':  return KEY_CANCEL;  // backspace -> back
    case 's': case 'S':    return KEY_SELECT;  // back-btn triple-click
    case 'h': case 'H':    return KEY_LEFT;    // vi-style fallbacks
    case 'l': case 'L':    return KEY_RIGHT;
    case 'k': case 'K':    return KEY_UP;
    case 'j': case 'J':    return KEY_DOWN;
    case 'p': case 'P':    return SIM_KEY_INJECT_PIN;
    case 'q': case 'Q':    return SIM_KEY_QUIT;
  }
  return 0;
}
