#pragma once
#include <stdint.h>
#include <helpers/ui/ListMenuScreen.h>
#include <helpers/ui/PinEntryScreen.h>
#include <helpers/child/ChildConfig.h>
#include <helpers/child/ChildMessageStore.h>
#include <helpers/child/ChildQuestion.h>
#include <helpers/child/ChildPresets.h>
#include <helpers/child/ChildAliases.h>
#include "ChildHomeScreen.h"
#include "ChildMessageScreen.h"
#include "ChildAlertScreen.h"
#include "ChildQuestionScreen.h"
#include "ChildBellScreen.h"
#include "ChildNoticeScreen.h"

#define CHILD_MAX_CHANNELS    8     // family groups shown first in the picker
#define CHILD_MAX_RECIPIENTS  24    // flat cap (NOT MAX_CONTACTS, which is 350 here)

namespace mesh { struct Identity; }
struct ContactInfo;

class ChildMode : public MenuHandler, public PinHandler, public ReaderHandler,
                  public AlertHandler, public QuestionHandler, public BellHandler,
                  public NoticeHandler {
  ChildConfig _cfg;
  ChildPresets _presets;
  ChildHomeScreen _home;
  ListMenuScreen _menu;
  PinEntryScreen _pin;
  ChildMessageStore _store;
  ChildMessageScreen _reader;
  ChildAlertScreen _alert;
  ChildQuestionScreen _question_screen;
  ChildNoticeScreen _notice;
  enum NoticeReturn { NR_HOME, NR_MENU, NR_SEND } _notice_return;
  ChildQuestion _question;            // parsed currently-open question
  int _question_msg_idx;
  struct ChildBellState {
    bool     is_channel;
    uint8_t  channel_idx;     // valid when is_channel
    uint8_t  pubkey[6];       // valid when !is_channel (caller, for the ack reply)
    uint32_t ring_until;      // millis() deadline for the ring window
  };
  ChildBellState  _bell;
  char            _bell_caller[32];
  ChildBellScreen _bell_screen;
  enum MenuContext { MENU_TOP, MENU_MESSAGES, MENU_SEND_PRESET, MENU_SEND_RECIPIENT };
  MenuContext _menu_context;
  struct ChildRecipient {
    bool    is_channel;
    uint8_t channel_idx;     // valid when is_channel
    uint8_t pubkey[6];       // valid when !is_channel
  };
  ChildRecipient _recips[CHILD_MAX_RECIPIENTS];
  int            _msg_icons[CHILD_MSG_CAP];   // per-row group/DM icon ids for the messages list
  ChildAliases   _aliases;                    // device-name -> display-name (for group messages)
  char           _recip_labels[CHILD_MAX_RECIPIENTS][32];
  const char*    _recip_label_ptrs[CHILD_MAX_RECIPIENTS];
  int            _recip_count;
  int            _send_slot;       // chosen preset slot (0-indexed)
  void loadOrSeed();
  void save();
  void loadPresets();
  void savePreset(int slot0);
  void loadAliases();
  void saveAliases();
  void openNotice(int icon, const char* line, uint32_t dismiss_ms, NoticeReturn ret);
  // group text "<sender>: <body>": if sender is aliased, rewrite into buf; else return text as-is.
  const char* aliasRewriteGroupText(const char* text, char* buf, int bufsz);
  void captureMessage(const char* origin, const char* text, uint32_t ts, bool is_channel,
                      const uint8_t* sender_prefix, uint8_t channel_idx, bool is_question);
  int  newestUnreadIndex() const;
  void openMessage(int idx);
  void openQuestion(int idx);
  void openSend();
  void raiseBell(const char* origin, bool is_channel, const uint8_t* pubkey, uint8_t channel_idx);
  void buildRecipients();
  void openRecipients();
public:
  ChildMode();
  void begin();
  UIScreen* getHomeScreen() { return &_home; }
  void openMenu();
  void openMessages();
  int  unreadCount() const { return _store.unread(); }
  int  tzOffsetMin() const { return _cfg.tz_offset_min; }   // minutes added to UTC
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
  // BellHandler
  void onBellRingTick() override;
  void onBellDismiss() override;
  // NoticeHandler
  void onNoticeDone() override;
};

extern ChildMode child_mode;
