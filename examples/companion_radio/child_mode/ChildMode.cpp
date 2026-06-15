#include "ChildMode.h"
#include <helpers/child/ChildCommands.h>
#include "../MyMesh.h"        // the_mesh
#include "../ui-new/UITask.h" // ui_task (extern), UITask API
#include <target.h>           // rtc_clock

#ifndef CHILD_DEFAULT_PIN
#define CHILD_DEFAULT_PIN 1234
#endif

// Read-ack text: "✓ seen" (UTF-8). Compile-time for now; OTA-configurable later.
#define CHILD_READ_ACK_TEXT "\xE2\x9C\x93 seen"

extern UITask ui_task;        // declared in main.cpp

static const uint8_t CHILD_KEY[7] = {'c','h','i','l','d',0,0};

// nRF52 DataStore::putBlobByKey rejects blobs shorter than PUB_KEY_SIZE+4+SIGNATURE_SIZE
static const int CHILD_BLOB_LEN = PUB_KEY_SIZE + 4 + SIGNATURE_SIZE;

// Top menu items
static const char* const MENU_ITEMS[] = { "Messages", "Settings", "Back" };
static const int MENU_COUNT = 3;
static const char* const NO_MSG_ITEMS[] = { "No messages" };

ChildMode child_mode;         // single global instance

ChildMode::ChildMode()
  : _home(this), _reader(this), _alert(this), _question_screen(this),
    _question_msg_idx(0), _menu_context(MENU_TOP) {}

void ChildMode::loadOrSeed() {
  uint8_t buf[CHILD_BLOB_LEN];
  int n = the_mesh.childGetBlob(CHILD_KEY, sizeof(CHILD_KEY), buf);
  if (n < CHILD_CONFIG_BLOB_LEN || !childConfigUnpack(_cfg, buf, n)) {
    childConfigInit(_cfg, (uint32_t)CHILD_DEFAULT_PIN);
    save();
  }
}

void ChildMode::save() {
  uint8_t buf[CHILD_BLOB_LEN];
  memset(buf, 0, sizeof(buf));
  childConfigPack(_cfg, buf);
  the_mesh.childPutBlob(CHILD_KEY, sizeof(CHILD_KEY), buf, CHILD_BLOB_LEN);
}

void ChildMode::begin() {
  loadOrSeed();
  ui_task.setInitialScreen(&_home);
}

void ChildMode::openMenu() {
  _menu_context = MENU_TOP;
  _menu.set("Menu", MENU_ITEMS, MENU_COUNT, this);
  ui_task.showScreen(&_menu);
}

void ChildMode::openMessages() {
  _menu_context = MENU_MESSAGES;
  if (_store.count() == 0) {
    _menu.set("Messages", NO_MSG_ITEMS, 1, this);
  } else {
    _menu.set("Messages", _store.labelPtrs(), _store.count(), this);
  }
  ui_task.showScreen(&_menu);
}

int ChildMode::newestUnreadIndex() const {
  for (int i = 0; i < _store.count(); i++) {
    if (childMsgPending(_store.at(i))) return i;
  }
  return 0;
}

void ChildMode::openQuestion(int idx) {
  const ChildMessage& m = _store.at(idx);
  parseChildQuestion(m.text, _question);
  _question_msg_idx = idx;
  int answered = (m.answer_idx == 0xFF) ? -1 : (int)m.answer_idx;
  _question_screen.open(&_question, answered);
  ui_task.showScreen(&_question_screen);
}

void ChildMode::openMessage(int idx) {
  if (_store.at(idx).is_question) { openQuestion(idx); return; }
  ChildReadResult r = _store.markRead(idx);
  if (r.transitioned) {
    if (r.is_channel) the_mesh.sendChildGroupText(r.channel_idx, CHILD_READ_ACK_TEXT);
    else              the_mesh.sendChildText(r.sender, CHILD_READ_ACK_TEXT);
  }
  _reader.open(&_store, idx);
  ui_task.showScreenAwake(&_reader);
}

void ChildMode::onMenuSelect(int idx) {
  if (_menu_context == MENU_MESSAGES) {
    if (_store.count() == 0) { openMenu(); return; }
    openMessage(idx);
    return;
  }
  if (idx == 0) {
    openMessages();
  } else if (idx == 1) {
    _pin.begin("Enter PIN", this);
    ui_task.showScreen(&_pin);
  } else {
    ui_task.showScreen(&_home);
  }
}

void ChildMode::onMenuCancel() {
  ui_task.showScreen(&_home);
}

void ChildMode::captureMessage(const char* origin, const char* text, uint32_t ts, bool is_channel,
                               const uint8_t* sender_prefix, uint8_t channel_idx, bool is_question) {
  _store.add(origin, text, ts, is_channel, sender_prefix, channel_idx, is_question);
  ui_task.notify(is_channel ? UIEventType::channelMessage : UIEventType::contactMessage);
  _alert.open(&_store);
  ui_task.showScreenAwake(&_alert);
}

void ChildMode::onReaderBack() {
  if (_store.count() > 0) openMessages();
  else ui_task.showScreen(&_home);
}

void ChildMode::onAlertOpen() {
  openMessage(newestUnreadIndex());
}

void ChildMode::onAlertDismiss() {
  ui_task.showScreen(&_home);
}

void ChildMode::onQuestionSelect(int opt) {
  const ChildMessage& m = _store.at(_question_msg_idx);
  const char* answer = _question.options[opt];
  bool sent = m.is_channel ? the_mesh.sendChildGroupText(m.channel_idx, answer)
                           : the_mesh.sendChildText(m.sender, answer);
  ui_task.showScreen(&_home);
  if (sent) {
    _store.markAnswered(_question_msg_idx, (uint8_t)opt);
    ui_task.showAlert("Sent", 1500);
  } else {
    ui_task.showAlert("Send failed", 1500);   // stays pending -> retryable
  }
}

void ChildMode::onQuestionCancel() {
  ui_task.showScreen(&_home);   // unanswered question stays pending (nag preserved)
}

void ChildMode::onPinEntered(uint32_t pin) {
  if (pin == _cfg.pin) {
    ui_task.showScreen(ui_task.getFullHomeScreen());
  } else {
    ui_task.showScreen(&_home);
  }
}

void ChildMode::onPinCancel() {
  ui_task.showScreen(&_home);
}

static_assert(ADV_TYPE_NONE == 0, "childSenderApproved assumes ADV_TYPE_NONE == 0");

bool ChildMode::onIncomingText(const ContactInfo& from, uint8_t txt_type,
                               uint32_t sender_timestamp, const char* text) {
  if (!childSenderApproved(from.type)) return true;   // drop strangers

  PinChange pc;
  if (parseChildCommand(text, pc) == CHILD_CMD_PIN_CHANGE) {
    if (pc.old_pin == _cfg.pin) { _cfg.pin = pc.new_pin; save(); }
    return true;
  }

  if (txt_type == TXT_TYPE_PLAIN || txt_type == TXT_TYPE_SIGNED_PLAIN) {
    ChildQuestion q;
    bool is_q = parseChildQuestion(text, q);
    captureMessage(from.name, text, sender_timestamp, false, from.id.pub_key, 0xFF, is_q);
    return true;
  }
  return false;
}

bool ChildMode::onIncomingChannel(uint8_t channel_idx, const char* channel_name,
                                  uint32_t timestamp, const char* text) {
  ChildQuestion q;
  bool is_q = parseChildQuestion(text, q);
  captureMessage(channel_name, text, timestamp, true, nullptr, channel_idx, is_q);
  return true;
}
