// Host filesystem shim: an Arduino-`File`/`fs::FS`-shaped layer backed by real
// files under sim/fsroot/. Selected via FILESYSTEM=HostFS when HOST_PLATFORM is
// defined (see IdentityStore.h). Part of sim/host/, not firmware.
#pragma once
#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <string>
#include <memory>
#include <sys/stat.h>
#include "Stream.h"   // Arduino File is-a Stream (Identity::readFrom/writeTo)

// Adafruit-LittleFS-style open-mode constants (host treats them as fopen modes).
#define FILE_O_READ  "r"
#define FILE_O_WRITE "w"

#ifndef SIM_FS_ROOT
#define SIM_FS_ROOT "sim/fsroot"
#endif

class HostFile : public Stream {
  std::shared_ptr<FILE> _f;
public:
  HostFile() {}
  explicit HostFile(FILE* f) : _f(f, [](FILE* p){ if (p) fclose(p); }) {}

  // own (Arduino File) API
  int read(uint8_t* buf, size_t n) { return _f ? (int)fread(buf, 1, n, _f.get()) : 0; }
  using Stream::read;   // keep the no-arg Stream::read() visible too
  bool seek(uint32_t pos) { return _f ? fseek(_f.get(), pos, SEEK_SET) == 0 : false; }
  uint32_t position() { return _f ? (uint32_t)ftell(_f.get()) : 0; }
  uint32_t size() {
    if (!_f) return 0;
    long cur = ftell(_f.get()); fseek(_f.get(), 0, SEEK_END);
    long end = ftell(_f.get()); fseek(_f.get(), cur, SEEK_SET);
    return (uint32_t)end;
  }
  void close() { _f.reset(); }
  explicit operator bool() const { return (bool)_f; }

  // directory-iteration API (companion has a debug file-lister); host = no entries
  HostFile openNextFile() { return HostFile(); }
  bool isDirectory() { return false; }
  const char* name() { return ""; }

  // Stream / Print overrides
  int read() override { if (!_f) return -1; int c = fgetc(_f.get()); return c == EOF ? -1 : c; }
  int peek() override { if (!_f) return -1; int c = fgetc(_f.get()); if (c != EOF) ungetc(c, _f.get()); return c == EOF ? -1 : c; }
  int available() override {
    if (!_f) return 0;
    long cur = ftell(_f.get()); fseek(_f.get(), 0, SEEK_END);
    long end = ftell(_f.get()); fseek(_f.get(), cur, SEEK_SET);
    return (int)(end - cur);
  }
  void flush() override { if (_f) fflush(_f.get()); }
  size_t write(uint8_t b) override { return _f ? (fputc(b, _f.get()), 1) : 0; }
  size_t write(const uint8_t* buf, size_t n) override { return _f ? fwrite(buf, 1, n, _f.get()) : 0; }
  using Print::write;
};

class HostFS {
  static std::string path(const char* p) {
    std::string s = SIM_FS_ROOT;
    if (p && p[0] != '/') s += '/';
    return s + (p ? p : "");
  }
  static const char* binMode(const char* mode) {
    if (!mode) return "rb";
    if (mode[0] == 'w') return "wb";
    if (mode[0] == 'a') return "ab";
    return "rb";
  }
  static HostFile openMode(const char* p, const char* mode) {
    return HostFile(fopen(path(p).c_str(), binMode(mode)));
  }
public:
  void begin() { ::mkdir(SIM_FS_ROOT, 0755); }

  HostFile open(const char* p) { return openMode(p, "r"); }
  HostFile open(const char* p, const char* mode) { return openMode(p, mode); }
  HostFile open(const char* p, const char* mode, bool /*create*/) { return openMode(p, mode); }

  bool exists(const char* p) { struct stat st; return ::stat(path(p).c_str(), &st) == 0; }
  bool remove(const char* p) { return ::remove(path(p).c_str()) == 0; }
  bool mkdir(const char* p) { int r = ::mkdir(path(p).c_str(), 0755); return r == 0 || errno == EEXIST; }
  bool format() { return true; }
};

// DataStore/IdentityStore use `File` unqualified (NRF52 gets it via a using-decl).
typedef HostFile File;
