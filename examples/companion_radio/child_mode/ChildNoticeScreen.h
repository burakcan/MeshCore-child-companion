#pragma once
#include <helpers/ui/UIScreen.h>

// ChildMode impls this: notice dismissed (timeout or key press).
class NoticeHandler {
public:
  virtual void onNoticeDone() = 0;
};

// Centered icon + one line. Auto-dismisses after dismiss_ms (0 = never); any key dismisses.
// Used for empty states and the "Sent" confirmation.
class ChildNoticeScreen : public UIScreen {
  NoticeHandler* _owner;
  int _icon;
  char _line[28];
  uint32_t _dismiss_ms;
  uint32_t _opened;
public:
  ChildNoticeScreen(NoticeHandler* owner)
    : _owner(owner), _icon(0), _dismiss_ms(0), _opened(0) { _line[0] = 0; }
  void open(int icon, const char* line, uint32_t dismiss_ms);
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
#ifdef CHILD_REMAP_LR_TO_UD
  bool isChildScreen() const override { return true; }
#endif
};
