#pragma once
#include <stdint.h>
#include <helpers/ui/ListMenuScreen.h>
#include <helpers/ui/PinEntryScreen.h>
#include <helpers/child/ChildConfig.h>
#include <helpers/child/ChildMessageStore.h>
#include <helpers/child/ChildQuestion.h>
#include "ChildHomeScreen.h"
#include "ChildMessageScreen.h"
#include "ChildAlertScreen.h"
#include "ChildQuestionScreen.h"

namespace mesh { struct Identity; }
struct ContactInfo;

class ChildMode : public MenuHandler, public PinHandler, public ReaderHandler,
                  public AlertHandler, public QuestionHandler {
  ChildConfig _cfg;
  ChildHomeScreen _home;
  ListMenuScreen _menu;
  PinEntryScreen _pin;
  ChildMessageStore _store;
  ChildMessageScreen _reader;
  ChildAlertScreen _alert;
  ChildQuestionScreen _question_screen;
  ChildQuestion _question;            // parsed view of the currently-open question
  int _question_msg_idx;
  enum MenuContext { MENU_TOP, MENU_MESSAGES };
  MenuContext _menu_context;
  void loadOrSeed();
  void save();
  void captureMessage(const char* origin, const char* text, uint32_t ts, bool is_channel,
                      const uint8_t* sender_prefix, uint8_t channel_idx, bool is_question);
  int  newestUnreadIndex() const;
  void openMessage(int idx);
  void openQuestion(int idx);
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
  // AlertHandler
  void onAlertOpen() override;
  void onAlertDismiss() override;
  // QuestionHandler
  void onQuestionSelect(int opt) override;
  void onQuestionCancel() override;
};

extern ChildMode child_mode;
