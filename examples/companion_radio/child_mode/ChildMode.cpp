#include "ChildMode.h"
#include <helpers/child/ChildCommands.h>
#include "../MyMesh.h"        // the_mesh
#include "../ui-new/UITask.h" // ui_task (extern), UITask API
#include <target.h>           // rtc_clock

#ifndef CHILD_DEFAULT_PIN
#define CHILD_DEFAULT_PIN 1234
#endif

extern UITask ui_task;        // declared in main.cpp

static const uint8_t CHILD_KEY[7] = {'c','h','i','l','d',0,0};

// nRF52 DataStore::putBlobByKey rejects blobs shorter than PUB_KEY_SIZE+4+SIGNATURE_SIZE
static const int CHILD_BLOB_LEN = PUB_KEY_SIZE + 4 + SIGNATURE_SIZE;

// Top menu items
static const char* const MENU_ITEMS[] = { "Messages", "Settings", "Back" };
static const int MENU_COUNT = 3;
static const char* const NO_MSG_ITEMS[] = { "No messages" };

ChildMode child_mode;         // single global instance

ChildMode::ChildMode() : _home(this), _reader(this), _menu_context(MENU_TOP),
                         _reader_return(RETURN_HOME) {}

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
  ui_task.setInitialScreen(&_home);     // boot straight into child UI
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

void ChildMode::onMenuSelect(int idx) {
  if (_menu_context == MENU_MESSAGES) {
    if (_store.count() == 0) { openMenu(); return; }   // "No messages" -> back to menu
    _reader_return = RETURN_LIST;
    _reader.open(&_store, idx);
    ui_task.showScreen(&_reader);
    return;
  }
  // MENU_TOP
  if (idx == 0) {                       // Messages
    openMessages();
  } else if (idx == 1) {                // Settings -> PIN gate
    _pin.begin("Enter PIN", this);
    ui_task.showScreen(&_pin);
  } else {                              // Back
    ui_task.showScreen(&_home);
  }
}

void ChildMode::onMenuCancel() {
  if (_menu_context == MENU_MESSAGES) {
    _store.markAllRead();
    ui_task.showScreen(&_home);
  } else {
    ui_task.showScreen(&_home);
  }
}

void ChildMode::captureMessage(const char* origin, const char* text, uint32_t ts, bool is_channel) {
  _store.add(origin, text, ts, is_channel);
  ui_task.notify(is_channel ? UIEventType::channelMessage : UIEventType::contactMessage);
  _reader_return = RETURN_HOME;
  _reader.open(&_store, 0);             // newest
  ui_task.showScreen(&_reader);
}

void ChildMode::onReaderBack() {
  if (_reader_return == RETURN_LIST) {
    openMessages();
  } else {
    _store.markAllRead();
    ui_task.showScreen(&_home);
  }
}

void ChildMode::onPinEntered(uint32_t pin) {
  if (pin == _cfg.pin) {
    ui_task.showScreen(ui_task.getFullHomeScreen());   // unlock full UI for session
  } else {
    ui_task.showScreen(&_home);                        // wrong -> back to child home
  }
}

void ChildMode::onPinCancel() {
  ui_task.showScreen(&_home);
}

static_assert(ADV_TYPE_NONE == 0, "childSenderApproved assumes ADV_TYPE_NONE == 0");

bool ChildMode::onIncomingText(const ContactInfo& from, uint8_t txt_type,
                               uint32_t sender_timestamp, const char* text) {
  // CHILD_MODE: only pre-approved contacts may message or command the child.
  if (!childSenderApproved(from.type)) return true;   // drop strangers

  PinChange pc;
  if (parseChildCommand(text, pc) == CHILD_CMD_PIN_CHANGE) {
    if (pc.old_pin == _cfg.pin) { _cfg.pin = pc.new_pin; save(); }
    return true;   // consume the command
  }

  // Capture approved displayable DMs into the child store, then suppress normal handling.
  if (txt_type == TXT_TYPE_PLAIN || txt_type == TXT_TYPE_SIGNED_PLAIN) {
    captureMessage(from.name, text, sender_timestamp, false);
    return true;
  }
  return false;   // e.g. CLI_DATA -> let normal firmware path handle it
}

bool ChildMode::onIncomingChannel(uint8_t channel_idx, const char* channel_name,
                                  uint32_t timestamp, const char* text) {
  (void)channel_idx;
  // Group membership == approved (parent controls the channel). Capture + suppress.
  captureMessage(channel_name, text, timestamp, true);
  return true;
}
