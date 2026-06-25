#pragma once
#include <helpers/ui/UIScreen.h>

class ChildMode;

class ChildHomeScreen : public UIScreen {
  ChildMode* _owner;
public:
  ChildHomeScreen(ChildMode* owner) : _owner(owner) {}
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
#ifdef CHILD_REMAP_LR_TO_UD
  bool isChildScreen() const override { return true; }
#endif
};
