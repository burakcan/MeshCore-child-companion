#pragma once
#include <stdint.h>
#include <helpers/ui/UIScreen.h>
#include <helpers/child/ChildMessageStore.h>

// ChildMode impls this: open newest unread, or dismiss to home.
class AlertHandler {
public:
  virtual void onAlertOpen() = 0;
  virtual void onAlertDismiss() = 0;
};

// Full-screen "new message" notification; kid opens manually.
class ChildAlertScreen : public UIScreen {
  AlertHandler* _owner;
  ChildMessageStore* _store;
  uint32_t _opened;            // ms alert appeared (arrival blink)
public:
  ChildAlertScreen(AlertHandler* owner) : _owner(owner), _store(nullptr), _opened(0) {}
  void open(ChildMessageStore* store);
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
#ifdef CHILD_REMAP_LR_TO_UD
  bool isChildScreen() const override { return true; }
#endif
};
