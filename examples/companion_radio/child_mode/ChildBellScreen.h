#pragma once
#include <helpers/ui/UIScreen.h>
#include <stdint.h>

#ifndef CHILD_BELL_RING_INTERVAL_MS
#define CHILD_BELL_RING_INTERVAL_MS 3500   // re-fire cadence; > ~3s tune => clean "ring ... ring ..."
#endif

class BellHandler {
public:
  virtual void onBellRingTick() = 0;   // re-ring while in window (no-op after)
  virtual void onBellDismiss() = 0;    // stop, ack caller, home
};

// Full-screen "<caller> is calling". ASCII only (CP437). Any key dismisses.
class ChildBellScreen : public UIScreen {
  BellHandler* _owner;
  char _caller[32];
  uint32_t _last_ring;
public:
  ChildBellScreen(BellHandler* owner) : _owner(owner), _last_ring(0) { _caller[0] = 0; }
  void open(const char* caller);
  int render(DisplayDriver& display) override;
  void poll() override;
  bool handleInput(char c) override;
};
