#pragma once
#include <stdint.h>
#include <helpers/ui/ListMenuScreen.h>
#include <helpers/ui/PinEntryScreen.h>
#include <helpers/child/ChildConfig.h>
#include <helpers/child/ChildMessageStore.h>
#include <helpers/child/ChildQuestion.h>
#include <helpers/child/ChildPresets.h>
#include "ChildHomeScreen.h"
#include "ChildMessageScreen.h"
#include "ChildAlertScreen.h"
#include "ChildQuestionScreen.h"

#define CHILD_MAX_CHANNELS    8     // family groups shown first in the picker
#define CHILD_MAX_RECIPIENTS  24    // flat cap (NOT MAX_CONTACTS, which is 350 here)

namespace mesh { struct Identity; }
struct ContactInfo;

class ChildMode : public MenuHandler, public PinHandler, public ReaderHandler,
                  public AlertHandler, public QuestionHandler {
  ChildConfig _cfg;
  ChildPresets _presets;
  ChildHomeScreen _home;
  ListMenuScreen _menu;
  PinEntryScreen _pin;
  ChildMessageStore _store;
  ChildMessageScreen _reader;
  ChildAlertScreen _alert;
  ChildQuestionScreen _question_screen;
  ChildQuestion _question;            // parsed view of the currently-open question
  int _question_msg_idx;
  enum MenuContext { MENU_TOP, MENU_MESSAGES, MENU_SEND_PRESET, MENU_SEND_RECIPIENT };
  MenuContext _menu_context;
  struct ChildRecipient {
    bool    is_channel;
    uint8_t channel_idx;     // valid when is_channel
    uint8_t pubkey[6];       // valid when !is_channel
  };
  ChildRecipient _recips[CHILD_MAX_RECIPIENTS];
  char           _recip_labels[CHILD_MAX_RECIPIENTS][32];
  const char*    _recip_label_ptrs[CHILD_MAX_RECIPIENTS];
  int            _recip_count;
  int            _send_slot;       // chosen preset slot (0-indexed)
  void loadOrSeed();
  void save();
  void loadPresets();
  void savePreset(int slot0);
  void captureMessage(const char* origin, const char* text, uint32_t ts, bool is_channel,
                      const uint8_t* sender_prefix, uint8_t channel_idx, bool is_question);
  int  newestUnreadIndex() const;
  void openMessage(int idx);
  void openQuestion(int idx);
  void openSend();
  void buildRecipients();
  void openRecipients();
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
