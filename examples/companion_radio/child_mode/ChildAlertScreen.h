#pragma once
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
public:
  ChildAlertScreen(AlertHandler* owner) : _owner(owner), _store(nullptr) {}
  void open(ChildMessageStore* store) { _store = store; }
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};
