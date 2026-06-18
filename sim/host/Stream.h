// Minimal Arduino Stream base for the host simulator.
#pragma once
#include "Print.h"

class Stream : public Print {
public:
  virtual int available() { return 0; }
  virtual int read() { return -1; }
  virtual int peek() { return -1; }
  virtual void flush() {}

  virtual size_t readBytes(uint8_t* buf, size_t len) {
    size_t n = 0;
    while (n < len) { int c = read(); if (c < 0) break; buf[n++] = (uint8_t)c; }
    return n;
  }
  size_t readBytes(char* buf, size_t len) { return readBytes((uint8_t*)buf, len); }

  // satisfy Print
  using Print::write;
};
