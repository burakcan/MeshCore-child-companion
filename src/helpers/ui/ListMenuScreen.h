#pragma once
#include "UIScreen.h"
#include "MenuModel.h"

class MenuHandler {
public:
  virtual void onMenuSelect(int idx) = 0;
  virtual void onMenuCancel() {}
};

class ListMenuScreen : public UIScreen {
  const char* _title;
  const char* const* _items;
  int _count;
  MenuHandler* _handler;
  const int* _icons = nullptr;   // per-row lead icon ids (UiIconId), -1 = none
  MenuModel _model;
public:
  ListMenuScreen() : _title(""), _items(nullptr), _count(0), _handler(nullptr) {}
  void set(const char* title, const char* const* items, int count, MenuHandler* h,
           const int* icons = nullptr);
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
#ifdef CHILD_REMAP_LR_TO_UD
  bool isChildScreen() const override { return true; }
#endif
};
