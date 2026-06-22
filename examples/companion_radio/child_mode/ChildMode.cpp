#include "ChildMode.h"
#include <helpers/ui/UiIcons.h>
#include <helpers/child/ChildCommands.h>
#include <helpers/child/ChildPresetCommand.h>
#include <helpers/child/ChildBell.h>
#include "../MyMesh.h"        // the_mesh
#include "../ui-new/UITask.h" // ui_task (extern), UITask API
#include <target.h>           // rtc_clock

#ifndef CHILD_DEFAULT_PIN
#define CHILD_DEFAULT_PIN 1234
#endif

// Read-ack text: "✓ seen" (UTF-8). Compile-time for now; OTA-configurable later.
#define CHILD_READ_ACK_TEXT "\xE2\x9C\x93 seen"

#ifndef CHILD_BELL_RING_MS
#define CHILD_BELL_RING_MS 45000      // ring window before auto-silencing (screen persists)
#endif
// "👋 here" (UTF-8); sent over radio only, never drawn on the CP437 OLED.
#define CHILD_BELL_ACK_TEXT "\xF0\x9F\x91\x8B here"

extern UITask ui_task;        // declared in main.cpp

static const uint8_t CHILD_KEY[7] = {'c','h','i','l','d',0,0};
static const uint8_t CHILD_ALIAS_KEY[7] = {'c','h','a','l','i',0,0};

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

ChildMode child_mode;         // single global instance

ChildMode::ChildMode()
  : _home(this), _reader(this), _alert(this), _question_screen(this), _notice(this),
    _notice_return(NR_HOME), _question_msg_idx(0), _bell_screen(this), _menu_context(MENU_TOP) {}

void ChildMode::openNotice(int icon, const char* line, uint32_t dismiss_ms, NoticeReturn ret) {
  _notice_return = ret;
  _notice.open(icon, line, dismiss_ms);
  ui_task.showScreenAwake(&_notice);
}

void ChildMode::onNoticeDone() {
  switch (_notice_return) {
    case NR_MENU: openMenu(); break;
    case NR_SEND: openSend(); break;
    case NR_HOME:
    default:      ui_task.showScreen(&_home); break;
  }
}

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

void ChildMode::loadAliases() {
  uint8_t buf[CHILD_ALIAS_SAVE_LEN];
  int n = the_mesh.childGetBlob(CHILD_ALIAS_KEY, sizeof(CHILD_ALIAS_KEY), buf);
  if (n > 0) _aliases.unpack(buf, n);
}

void ChildMode::saveAliases() {
  uint8_t buf[CHILD_ALIAS_SAVE_LEN];
  _aliases.pack(buf);
  the_mesh.childPutBlob(CHILD_ALIAS_KEY, sizeof(CHILD_ALIAS_KEY), buf, CHILD_ALIAS_SAVE_LEN);
}

const char* ChildMode::aliasRewriteGroupText(const char* text, char* buf, int bufsz) {
  const char* sep = strstr(text, ": ");          // "<sender>: <body>"
  if (sep == NULL) return text;
  int nlen = (int)(sep - text);
  if (nlen <= 0 || nlen >= CHILD_ALIAS_NAMELEN) return text;
  char sender[CHILD_ALIAS_NAMELEN];
  memcpy(sender, text, nlen); sender[nlen] = 0;
  const char* disp = _aliases.lookup(sender);
  if (disp == NULL) return text;
  snprintf(buf, bufsz, "%s%s", disp, sep);       // "<display>: <body>"
  return buf;
}

void ChildMode::begin() {
  loadOrSeed();
  loadPresets();
  loadAliases();
  ui_task.setInitialScreen(&_home);
}

void ChildMode::openMenu() {
  _menu_context = MENU_TOP;
  static const int MENU_ICONS[] = { ICON_ENVELOPE_SM, ICON_SEND_SM, ICON_GEAR_SM, ICON_CHEVRON_LEFT };
  _menu.set("Menu", MENU_ITEMS, MENU_COUNT, this, MENU_ICONS);
  ui_task.showScreen(&_menu);
}

void ChildMode::openMessages() {
  if (_store.count() == 0) { openNotice(ICON_ENVELOPE, "No messages yet", 0, NR_HOME); return; }
  _menu_context = MENU_MESSAGES;
  // per-row icon: group marker for channel msgs; DMs have none (sender name in label suffices)
  for (int i = 0; i < _store.count(); i++) {
    _msg_icons[i] = _store.at(i).is_channel ? ICON_GROUP_SM : -1;
  }
  _menu.set("Messages", _store.labelPtrs(), _store.count(), this, _msg_icons);
  ui_task.showScreen(&_menu);
}

void ChildMode::openSend() {
  const char* const* items = _presets.labelPtrs();   // also rebuilds labelCount()
  if (_presets.labelCount() == 0) { openNotice(ICON_SEND, "No quick messages", 0, NR_MENU); return; }
  _menu_context = MENU_SEND_PRESET;
  _menu.set("Send", items, _presets.labelCount(), this);
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
  if (_recip_count == 0) { openNotice(ICON_PERSON, "No one to send to", 0, NR_MENU); return; }
  _menu_context = MENU_SEND_RECIPIENT;
  _menu.set("Send to", _recip_label_ptrs, _recip_count, this);
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
  _question_screen.open(&_question, answered, m.origin);
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
    _send_slot = _presets.slotForRow(idx);
    buildRecipients();
    openRecipients();
    return;
  }
  if (_menu_context == MENU_SEND_RECIPIENT) {
    const ChildRecipient& r = _recips[idx];
    const char* txt = _presets.text(_send_slot);
    bool ok = r.is_channel ? the_mesh.sendChildGroupText(r.channel_idx, txt)
                           : the_mesh.sendChildText(r.pubkey, txt);
    if (ok) {
      openNotice(ICON_CHECK, "Sent", 1200, NR_HOME);   // check flourish, then home
    } else {
      ui_task.showAlert("Send failed", 1500);   // stays on recipient list -> retryable
    }
    return;
  }
  if (_menu_context == MENU_MESSAGES) {
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
  if (sent) {
    _store.markAnswered(_question_msg_idx, (uint8_t)opt);
    openNotice(ICON_CHECK, "Sent", 1200, NR_HOME);
  } else {
    ui_task.showScreen(&_home);
    ui_task.showAlert("Send failed", 1500);   // stays pending -> retryable
  }
}

void ChildMode::onQuestionCancel() {
  ui_task.showScreen(&_home);   // unanswered question stays pending (nag preserved)
}

void ChildMode::raiseBell(const char* origin, bool is_channel, const uint8_t* pubkey,
                          uint8_t channel_idx) {
  _bell.is_channel = is_channel;
  _bell.channel_idx = channel_idx;
  if (pubkey) memcpy(_bell.pubkey, pubkey, 6);
  _bell.ring_until = millis() + CHILD_BELL_RING_MS;
  strncpy(_bell_caller, origin ? origin : "", sizeof(_bell_caller) - 1);
  _bell_caller[sizeof(_bell_caller) - 1] = 0;
  _bell_screen.open(_bell_caller);
  ui_task.notify(UIEventType::bell);          // first ring now; screen re-fires within the window
  ui_task.showScreenAwake(&_bell_screen);     // wake + show (mirrors the message alert)
}

void ChildMode::onBellRingTick() {
  if (millis() < _bell.ring_until) ui_task.notify(UIEventType::bell);
}

void ChildMode::onBellDismiss() {
  // deliberate dismiss -> ack the caller. timed-out/ignored bell sends nothing.
  if (_bell.is_channel) the_mesh.sendChildGroupText(_bell.channel_idx, CHILD_BELL_ACK_TEXT);
  else                  the_mesh.sendChildText(_bell.pubkey, CHILD_BELL_ACK_TEXT);
  ui_task.showScreen(&_home);
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

  int tz;
  if (parseTzCommand(text, &tz)) {                          // !tz <minutes>: clock offset
    _cfg.tz_offset_min = (int16_t)tz; save();
    return true;
  }

  char newname[32];
  if (parseNameCommand(text, newname, sizeof(newname))) {   // !name <x>: sender renames itself
    char oldname[32];                                       // current name = its group on-air name
    strncpy(oldname, from.name, sizeof(oldname) - 1); oldname[sizeof(oldname) - 1] = 0;
    the_mesh.childSetContactName(from.id.pub_key, newname); // rename DM contact (+ full UI)
    _aliases.set(oldname, newname);                         // alias embedded name for group msgs
    saveAliases();
    return true;
  }

  if (parseChildBell(text)) {
    raiseBell(from.name, false, from.id.pub_key, 0xFF);
    return true;                               // ephemeral: not stored, no unread badge
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
  if (parseChildBell(text)) {
    raiseBell(channel_name, true, nullptr, channel_idx);
    return true;
  }

  // rewrite embedded "<sender>: " to its alias so !name reaches groups too
  char rewritten[CHILD_MSG_BODY];
  const char* disp_text = aliasRewriteGroupText(text, rewritten, sizeof(rewritten));

  ChildQuestion q;
  bool is_q = parseChildQuestion(disp_text, q);
  captureMessage(channel_name, disp_text, timestamp, true, nullptr, channel_idx, is_q);
  return true;
}
