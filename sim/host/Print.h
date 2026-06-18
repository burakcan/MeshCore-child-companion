// Minimal Arduino Print base for the host simulator.
#pragma once
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <cstdarg>

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

class Print {
public:
  virtual ~Print() {}
  virtual size_t write(uint8_t) = 0;
  virtual size_t write(const uint8_t* buf, size_t size) {
    size_t n = 0; while (size--) n += write(*buf++); return n;
  }
  size_t write(const char* s) { return s ? write((const uint8_t*)s, strlen(s)) : 0; }

  size_t print(const char* s) { return write(s); }
  size_t print(char c) { return write((uint8_t)c); }
  size_t print(unsigned char v, int base = DEC) { return print((unsigned long)v, base); }
  size_t print(int v, int base = DEC) { return base == DEC ? printSigned(v) : print((unsigned long)v, base); }
  size_t print(unsigned int v, int base = DEC) { return print((unsigned long)v, base); }
  size_t print(long v, int base = DEC) { return base == DEC ? printSigned(v) : print((unsigned long)v, base); }
  size_t print(double v) { char b[32]; int n = snprintf(b, sizeof b, "%g", v); return write((const uint8_t*)b, n); }
  size_t print(unsigned long v, int base = DEC) {
    char b[34]; const char* fmt = base == HEX ? "%lx" : base == OCT ? "%lo" : "%lu";
    int n = snprintf(b, sizeof b, fmt, v); return write((const uint8_t*)b, n);
  }

  // some firmware uses Stream::printf (ESP/RP cores provide it); route to write()
  int printf(const char* fmt, ...) {
    char buf[256]; va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
    if (n > 0) write((const uint8_t*)buf, n > (int)sizeof buf ? sizeof buf : n);
    return n;
  }

  size_t println() { return write((const uint8_t*)"\r\n", 2); }
  template <typename T> size_t println(T v) { size_t n = print(v); return n + println(); }
  template <typename T> size_t println(T v, int base) { size_t n = print(v, base); return n + println(); }

private:
  size_t printSigned(long v) { char b[24]; int n = snprintf(b, sizeof b, "%ld", v); return write((const uint8_t*)b, n); }
};
