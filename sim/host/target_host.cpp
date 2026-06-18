// Host equivalent of variants/<board>/target.cpp: defines the globals the
// firmware expects (board, radio_driver, rtc_clock, sensors) using host types.
#include <target.h>
#include <helpers/ArduinoHelpers.h>

HostBoard        board;
VirtualRadio     radio_driver;
VolatileRTCClock rtc_clock;
SensorManager    sensors;

#if defined(PIN_USER_BTN)
#if defined(UI_HAS_JOYSTICK)
MomentaryButton user_btn(PIN_USER_BTN, 1000, true, false, false);   // joystick center-press
MomentaryButton joystick_left(JOYSTICK_LEFT, 1000, true, false, false);
MomentaryButton joystick_right(JOYSTICK_RIGHT, 1000, true, false, false);
MomentaryButton back_btn(PIN_BACK_BTN, 1000, true, false, true);
#if defined(UI_HAS_JOYSTICK_UPDOWN)
MomentaryButton joystick_up(JOYSTICK_UP, 1000, true, false, false);
MomentaryButton joystick_down(JOYSTICK_DOWN, 1000, true, false, false);
#endif
#else
MomentaryButton user_btn(PIN_USER_BTN, 1000, true);   // single button: multi-click on (Heltec V3 style)
#endif
#endif

static StdRNG _id_rng;
mesh::LocalIdentity radio_new_identity() {
  _id_rng.begin((long)millis());
  return mesh::LocalIdentity(&_id_rng);
}
