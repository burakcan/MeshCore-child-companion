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

// Menu items
static const char* const MENU_ITEMS[] = { "Settings", "Back" };
static const int MENU_COUNT = 2;

ChildMode child_mode;         // single global instance

ChildMode::ChildMode() : _home(this) {}

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
  _menu.set("Menu", MENU_ITEMS, MENU_COUNT, this);
  ui_task.showScreen(&_menu);
}

void ChildMode::onMenuSelect(int idx) {
  if (idx == 0) {                       // Settings -> PIN gate
    _pin.begin("Enter PIN", this);
    ui_task.showScreen(&_pin);
  } else {                              // Back
    ui_task.showScreen(&_home);
  }
}

void ChildMode::onMenuCancel() {
  ui_task.showScreen(&_home);
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

bool ChildMode::onIncomingText(const ContactInfo& from, uint8_t txt_type, const char* text) {
  PinChange pc;
  if (parseChildCommand(text, pc) == CHILD_CMD_PIN_CHANGE) {
    if (pc.old_pin == _cfg.pin) {
      _cfg.pin = pc.new_pin;
      save();
    }
    return true;   // consume the command either way (don't surface as a message)
  }
  return false;
}
