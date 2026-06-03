#pragma once
#include <stdint.h>
#include <helpers/ui/ListMenuScreen.h>
#include <helpers/ui/PinEntryScreen.h>
#include <helpers/child/ChildConfig.h>
#include "ChildHomeScreen.h"

namespace mesh { struct Identity; }
struct ContactInfo;

class ChildMode : public MenuHandler, public PinHandler {
  ChildConfig _cfg;
  ChildHomeScreen _home;
  ListMenuScreen _menu;
  PinEntryScreen _pin;
  void loadOrSeed();
  void save();
public:
  ChildMode();
  void begin();                       // load config, register initial screen with UITask
  UIScreen* getHomeScreen() { return &_home; }
  void openMenu();                    // called by ChildHomeScreen on press
  bool onIncomingText(const ContactInfo& from, uint8_t txt_type, const char* text);

  // MenuHandler
  void onMenuSelect(int idx) override;
  void onMenuCancel() override;
  // PinHandler
  void onPinEntered(uint32_t pin) override;
  void onPinCancel() override;
};

extern ChildMode child_mode;          // defined in ChildMode.cpp
