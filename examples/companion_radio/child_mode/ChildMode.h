#pragma once
#include <stdint.h>
#include <helpers/ui/ListMenuScreen.h>
#include <helpers/ui/PinEntryScreen.h>
#include <helpers/child/ChildConfig.h>
#include <helpers/child/ChildMessageStore.h>
#include "ChildHomeScreen.h"
#include "ChildMessageScreen.h"

namespace mesh { struct Identity; }
struct ContactInfo;

class ChildMode : public MenuHandler, public PinHandler, public ReaderHandler {
  ChildConfig _cfg;
  ChildHomeScreen _home;
  ListMenuScreen _menu;
  PinEntryScreen _pin;
  ChildMessageStore _store;
  ChildMessageScreen _reader;
  enum MenuContext { MENU_TOP, MENU_MESSAGES };
  MenuContext _menu_context;
  enum ReaderReturn { RETURN_HOME, RETURN_LIST };
  ReaderReturn _reader_return;
  void loadOrSeed();
  void save();
  void captureMessage(const char* origin, const char* text, uint32_t ts, bool is_channel);
public:
  ChildMode();
  void begin();
  UIScreen* getHomeScreen() { return &_home; }
  void openMenu();
  void openMessages();
  int  unreadCount() const { return _store.unread(); }
  bool onIncomingText(const ContactInfo& from, uint8_t txt_type, uint32_t sender_timestamp,
                      const char* text);
  bool onIncomingChannel(uint8_t channel_idx, const char* channel_name, uint32_t timestamp,
                         const char* text);
  // MenuHandler
  void onMenuSelect(int idx) override;
  void onMenuCancel() override;
  // PinHandler
  void onPinEntered(uint32_t pin) override;
  void onPinCancel() override;
  // ReaderHandler
  void onReaderBack() override;
};

extern ChildMode child_mode;
