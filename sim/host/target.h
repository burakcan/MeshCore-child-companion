// Host substitute for variants/<board>/target.h. Resolved first via -I sim/host
// so MyMesh's `#include <target.h>` gets host globals instead of RadioLib + the
// nRF52 board + real sensors. Part of the full simulator (sim/host/).
#pragma once

#include <Arduino.h>
#include <helpers/ArduinoHelpers.h>     // VolatileRTCClock, ArduinoMillis, StdRNG
#include <helpers/SensorManager.h>      // base SensorManager (concrete, no-op)
#include "HostBoard.h"
#include "VirtualRadio.h"

// L1-style joystick pin map (the sim is board-agnostic; these are just ids the
// host digitalRead() bridge maps keyboard keys onto). Only for joystick builds;
// single-button builds pass their own PIN_USER_BTN via -D (e.g. Heltec V3 = 0).
#if defined(UI_HAS_JOYSTICK) && !defined(PIN_BUTTON1)
#define PIN_BUTTON1   13   // back / menu
#define PIN_BUTTON2   25   // joystick up
#define PIN_BUTTON3   26   // joystick down
#define PIN_BUTTON4   27   // joystick left
#define PIN_BUTTON5   28   // joystick right
#define PIN_BUTTON6   29   // joystick press
#define PIN_BACK_BTN  PIN_BUTTON1
#define JOYSTICK_UP    PIN_BUTTON2
#define JOYSTICK_DOWN  PIN_BUTTON3
#define JOYSTICK_LEFT  PIN_BUTTON4
#define JOYSTICK_RIGHT PIN_BUTTON5
#define PIN_USER_BTN   PIN_BUTTON6
#endif

#if defined(PIN_USER_BTN)
#include <helpers/ui/MomentaryButton.h>
extern MomentaryButton user_btn;            // single button OR joystick center-press
#if defined(UI_HAS_JOYSTICK)
extern MomentaryButton joystick_left;
extern MomentaryButton joystick_right;
extern MomentaryButton back_btn;
#if defined(UI_HAS_JOYSTICK_UPDOWN)
extern MomentaryButton joystick_up;
extern MomentaryButton joystick_down;
#endif
#endif
#endif

// Globals the firmware expects target.cpp to define (see sim/host/target_host.cpp).
extern HostBoard       board;
extern VirtualRadio    radio_driver;    // the mesh::Radio
extern VolatileRTCClock rtc_clock;
extern SensorManager   sensors;

mesh::LocalIdentity radio_new_identity();   // create a fresh random identity
