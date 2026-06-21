// Host implementations of the Arduino core shim (timing, rng, gpio no-ops).
#include "Arduino.h"
#include <chrono>
#include <thread>

static std::chrono::steady_clock::time_point g_t0 = std::chrono::steady_clock::now();

unsigned long millis() {
  auto now = std::chrono::steady_clock::now();
  return (unsigned long)std::chrono::duration_cast<std::chrono::milliseconds>(now - g_t0).count();
}
unsigned long micros() {
  auto now = std::chrono::steady_clock::now();
  return (unsigned long)std::chrono::duration_cast<std::chrono::microseconds>(now - g_t0).count();
}
void delay(unsigned long ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
void delayMicroseconds(unsigned long us) { std::this_thread::sleep_for(std::chrono::microseconds(us)); }

long random(long howbig) { return howbig > 0 ? (long)(std::rand() % howbig) : 0; }
long random(long howsmall, long howbig) {
  return howbig <= howsmall ? howsmall : howsmall + random(howbig - howsmall);
}
void randomSeed(unsigned long seed) { std::srand((unsigned)seed); }

// --- keyboard-driven button bridge ---------------------------------------
// Active-low buttons read HIGH (released) by default; hostPressPin() drives a
// pin LOW for a short window to simulate a click. Lets MomentaryButton (and the
// real UITask joystick handler) respond to the PC keyboard.
static unsigned long s_pin_release[64] = {0};
void hostPressPin(uint32_t pin, unsigned long dur_ms) {
  if (pin < 64) s_pin_release[pin] = millis() + dur_ms;
}
// Hold a button down until released (lets a real key-hold become a long-press).
void hostHoldPin(uint32_t pin)    { if (pin < 64) s_pin_release[pin] = millis() + 3600000UL; }
void hostReleasePin(uint32_t pin) { if (pin < 64) s_pin_release[pin] = 0; }

void pinMode(uint32_t, uint8_t) {}
void digitalWrite(uint32_t, uint8_t) {}
int  digitalRead(uint32_t pin) {
  if (pin < 64 && millis() < s_pin_release[pin]) return LOW;   // pressed
  return HIGH;                                                 // released
}
int  analogRead(uint32_t) { return 0; }
void analogWrite(uint32_t, int) {}
void yield() {}

HostSerial Serial;
