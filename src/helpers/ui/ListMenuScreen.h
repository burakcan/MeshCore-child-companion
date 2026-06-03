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
  MenuModel _model;
public:
  ListMenuScreen() : _title(""), _items(nullptr), _count(0), _handler(nullptr) {}
  void set(const char* title, const char* const* items, int count, MenuHandler* h);
  int render(DisplayDriver& display) override;
  bool handleInput(char c) override;
};
