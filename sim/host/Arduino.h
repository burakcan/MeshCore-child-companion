// Host substitute for <Arduino.h>: just enough for the MeshCore core +
// companion firmware to build and run on a PC. Part of the full simulator
// harness (sim/host/), not firmware.
#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <cstdarg>
#include "Print.h"
#include "Stream.h"

// Arduino min/max as global 2-type templates (NOT macros; macros break the STL,
// which uses std::min/std::max internally). Bare min()/max() in firmware bind here.
template <typename T, typename U> inline auto min(T a, U b) -> decltype(a < b ? a : b) { return a < b ? a : b; }
template <typename T, typename U> inline auto max(T a, U b) -> decltype(a > b ? a : b) { return a > b ? a : b; }

typedef uint8_t byte;
typedef bool boolean;

#ifndef PROGMEM
#define PROGMEM
#endif
#define F(x) (x)
#define PSTR(x) (x)
#define pgm_read_byte(addr)  (*(const uint8_t*)(addr))
#define pgm_read_word(addr)  (*(const uint16_t*)(addr))
#define pgm_read_dword(addr) (*(const uint32_t*)(addr))
#define pgm_read_ptr(addr)   (*(void* const*)(addr))
#define pgm_read_float(addr) (*(const float*)(addr))
#define memcpy_P memcpy
#define strcpy_P strcpy
#define strlen_P strlen

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define INPUT_PULLDOWN 3
#define LSBFIRST 0
#define MSBFIRST 1

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif

#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
// NB: deliberately NOT defining Arduino's map() macro; it collides with std::map.
static inline long arduino_map(long x, long a, long b, long c, long d) { return (x - a) * (d - c) / (b - a) + c; }
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))

// timing (defined in arduino.cpp)
unsigned long millis();
unsigned long micros();
void delay(unsigned long ms);
void delayMicroseconds(unsigned long us);

// rng
long random(long howbig);
long random(long howsmall, long howbig);
void randomSeed(unsigned long seed);

// gpio / misc (no-ops on host)
void pinMode(uint32_t pin, uint8_t mode);
void digitalWrite(uint32_t pin, uint8_t val);
int  digitalRead(uint32_t pin);
int  analogRead(uint32_t pin);
void analogWrite(uint32_t pin, int val);
void yield();

// host simulator: drive a button pin LOW (pressed) for dur_ms (keyboard bridge)
void hostPressPin(uint32_t pin, unsigned long dur_ms = 90);

template <typename T> T abs_(T v) { return v < 0 ? -v : v; }

// AVR-ism used by some helpers
inline char* ltoa(long v, char* buf, int base) {
  if (base == 16) sprintf(buf, "%lx", v);
  else if (base == 8) sprintf(buf, "%lo", v);
  else sprintf(buf, "%ld", v);
  return buf;
}
inline char* itoa(int v, char* buf, int base) { return ltoa(v, buf, base); }
inline char* utoa(unsigned v, char* buf, int base) {
  if (base == 16) sprintf(buf, "%x", v); else sprintf(buf, "%u", v); return buf;
}

// Arduino's serial. Writes go to stdout; reads return nothing by default.
class HostSerial : public Stream {
public:
  void begin(unsigned long = 0) {}
  void end() {}
  size_t write(uint8_t c) override { return fputc(c, stdout), 1; }
  using Print::write;
  int printf(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt); int n = vprintf(fmt, ap); va_end(ap); return n;
  }
  operator bool() const { return true; }
};
extern HostSerial Serial;
