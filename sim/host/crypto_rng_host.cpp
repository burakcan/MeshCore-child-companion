// Host replacement for rweather Crypto's hardware-bound RNG.cpp. Provides the
// global `RNG` object that Ed25519.cpp references (only in key generation, which
// the simulator doesn't use; we keygen via lib/ed25519). rand() is host-backed
// so it still works if ever called.
#include "RNG.h"
#include <cstdlib>

RNGClass RNG;

RNGClass::RNGClass() : credits(0), firstSave(1), initialized(0), trngPending(0),
                       timer(0), timeout(0), count(0), trngPosn(0) {}
RNGClass::~RNGClass() {}

void RNGClass::begin(const char*) {}
void RNGClass::addNoiseSource(NoiseSource&) {}
void RNGClass::setAutoSaveTime(uint16_t) {}
void RNGClass::rand(uint8_t* data, size_t len) { for (size_t i = 0; i < len; i++) data[i] = (uint8_t)std::rand(); }
bool RNGClass::available(size_t) const { return true; }
void RNGClass::stir(const uint8_t*, size_t, unsigned int) {}
void RNGClass::save() {}
void RNGClass::loop() {}
void RNGClass::destroy() {}
