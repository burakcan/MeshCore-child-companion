#include "ChildMode.h"
#include <helpers/child/ChildCommands.h>
#include <helpers/child/ChildPresetCommand.h>
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

// per-slot preset blob key: "chpre" + slot index (7 bytes). distinct from CHILD_KEY.
static void childPresetKey(uint8_t slot, uint8_t key[7]) {
  key[0] = 'c'; key[1] = 'h'; key[2] = 'p'; key[3] = 'r'; key[4] = 'e';
  key[5] = slot; key[6] = 0;
}
// DataStore rejects blobs < 100 bytes; pad each preset record to the minimum.
static const int CHILD_PRESET_SAVE_LEN = PUB_KEY_SIZE + 4 + SIGNATURE_SIZE;  // 100

static const char* const MENU_ITEMS[] = { "Messages", "Send", "Settings", "Back" };
static const int MENU_COUNT = 4;
static const char* const NO_MSG_ITEMS[] = { "No messages" };
static const char* const NO_PRESET_ITEMS[] = { "No presets" };
static const char* const NO_RECIP_ITEMS[]  = { "No recipients" };

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

void ChildMode::loadPresets() {
  uint8_t buf[CHILD_PRESET_SAVE_LEN];
  uint8_t key[7];
  bool any = false;
  for (int i = 0; i < CHILD_PRESET_SLOTS; i++) {
    childPresetKey((uint8_t)i, key);
    int n = the_mesh.childGetBlob(key, sizeof(key), buf);
    if (n > 0) { any = true; _presets.unpackSlot(i, buf, n); }
  }
  if (!any) {                       // fresh device: seed + persist defaults
    _presets.initDefaults();
    for (int i = 0; i < CHILD_PRESET_SLOTS; i++) {
      if (!_presets.isEmpty(i)) savePreset(i);
    }
  }
}

void ChildMode::savePreset(int slot0) {
  uint8_t buf[CHILD_PRESET_SAVE_LEN];
  uint8_t key[7];
  memset(buf, 0, sizeof(buf));
  _presets.packSlot(slot0, buf);    // rest stays zero-padded to the 100-byte min
  childPresetKey((uint8_t)slot0, key);
  the_mesh.childPutBlob(key, sizeof(key), buf, CHILD_PRESET_SAVE_LEN);
}

void ChildMode::begin() {
  loadOrSeed();
  loadPresets();
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

void ChildMode::openSend() {
  _menu_context = MENU_SEND_PRESET;
  const char* const* items = _presets.labelPtrs();   // rebuilds labelCount()
  if (_presets.labelCount() == 0) _menu.set("Send", NO_PRESET_ITEMS, 1, this);
  else                            _menu.set("Send", items, _presets.labelCount(), this);
  ui_task.showScreen(&_menu);
}

void ChildMode::buildRecipients() {
  _recip_count = 0;
  // group channels first (capped at CHILD_MAX_CHANNELS)
  int nch = 0;
  for (int ci = 0; ci < MAX_GROUP_CHANNELS && nch < CHILD_MAX_CHANNELS
                   && _recip_count < CHILD_MAX_RECIPIENTS; ci++) {
    ChannelDetails ch;
    if (the_mesh.getChannel(ci, ch) && ch.name[0]) {
      _recips[_recip_count].is_channel = true;
      _recips[_recip_count].channel_idx = (uint8_t)ci;
      strncpy(_recip_labels[_recip_count], ch.name, 31);
      _recip_labels[_recip_count][31] = 0;
      _recip_count++;
      nch++;
    }
  }
  // then contacts, in existing sort order
  int nc = the_mesh.getNumContacts();
  for (int i = 0; i < nc && _recip_count < CHILD_MAX_RECIPIENTS; i++) {
    ContactInfo c;
    if (the_mesh.getContactByIdx((uint32_t)i, c)) {
      _recips[_recip_count].is_channel = false;
      memcpy(_recips[_recip_count].pubkey, c.id.pub_key, 6);
      strncpy(_recip_labels[_recip_count], c.name, 31);
      _recip_labels[_recip_count][31] = 0;
      _recip_count++;
    }
  }
  if (_recip_count >= CHILD_MAX_RECIPIENTS) {
    MESH_DEBUG_PRINTLN("child: recipient list capped at %d", CHILD_MAX_RECIPIENTS);
  }
  for (int i = 0; i < _recip_count; i++) _recip_label_ptrs[i] = _recip_labels[i];
}

void ChildMode::openRecipients() {
  _menu_context = MENU_SEND_RECIPIENT;
  if (_recip_count == 0) {
    _menu.set("Send to", NO_RECIP_ITEMS, 1, this);
  } else {
    _menu.set("Send to", _recip_label_ptrs, _recip_count, this);
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
  // empty preset/recipient/message lists never reach here; open*() divert to a notice
  if (_menu_context == MENU_SEND_PRESET) {
    if (_presets.labelCount() == 0) { openMenu(); return; }   // "No presets" -> back to menu
    _send_slot = _presets.slotForRow(idx);
    buildRecipients();
    openRecipients();
    return;
  }
  if (_menu_context == MENU_SEND_RECIPIENT) {
    if (_recip_count == 0) { ui_task.showScreen(&_home); return; }  // "No recipients"
    const ChildRecipient& r = _recips[idx];
    const char* txt = _presets.text(_send_slot);
    bool ok = r.is_channel ? the_mesh.sendChildGroupText(r.channel_idx, txt)
                           : the_mesh.sendChildText(r.pubkey, txt);
    if (ok) {
      ui_task.showScreen(&_home);
      ui_task.showAlert("Sent", 1500);
    } else {
      ui_task.showAlert("Send failed", 1500);   // stays on recipient list -> retryable
    }
    return;
  }
  if (_menu_context == MENU_MESSAGES) {
    if (_store.count() == 0) { openMenu(); return; }
    openMessage(idx);
    return;
  }
  if (idx == 0) {
    openMessages();
  } else if (idx == 1) {
    openSend();
  } else if (idx == 2) {
    _pin.begin("Enter PIN", this);
    ui_task.showScreen(&_pin);
  } else {
    ui_task.showScreen(&_home);
  }
}

void ChildMode::onMenuCancel() {
  if (_menu_context == MENU_SEND_RECIPIENT) { openSend(); return; }  // -> preset list
  if (_menu_context == MENU_SEND_PRESET)    { openMenu(); return; }  // -> top menu
  ui_task.showScreen(&_home);   // MENU_TOP / MENU_MESSAGES -> home
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

  int pslot; char ptext[CHILD_PRESET_TEXTLEN];
  if (parsePresetCommand(text, pslot, ptext, sizeof(ptext))) {
    _presets.set(pslot - 1, ptext[0] ? ptext : nullptr);   // empty => clear
    savePreset(pslot - 1);
    return true;                                            // silent, like !pin
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
