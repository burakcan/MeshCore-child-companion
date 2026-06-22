#!/bin/sh
# Full firmware-on-host simulator. Profile-driven so it runs different ROLES
# (companion / repeater / room_server), not just one board. All hardware is
# uniformly virtualized (HostBoard/VirtualRadio/HostFS/host clocks), so the
# physical board is irrelevant; a "profile" only selects app + flags + UI.
#
# Usage:  sh sim/build-full.sh [companion|repeater|min]
set -e
cd "$(dirname "$0")/.."   # repo root
PROFILE="${1:-companion}"

# --- locate PlatformIO-fetched libs (any prior firmware build provides them) ---
# Use shell globs, NOT `find`: .pio/libdeps is ~11GB / 100k+ files (one copy per
# build env), and a recursive scan can take minutes (looks like a hang). Globs
# resolve instantly. Override any of these by exporting the var before running.
firstdir() { for d in $1; do [ -d "$d" ] && { printf '%s\n' "$d"; return; }; done; }
: "${CRYPTO:=$(firstdir '.pio/libdeps/*/Crypto')}"
: "${LPP:=$(firstdir '.pio/libdeps/*/CayenneLPP/src')}"
: "${B64:=$(firstdir '.pio/libdeps/*/base64/src')}"
for v in CRYPTO LPP B64; do
  eval "p=\$$v"
  [ -n "$p" ] || { echo "ERROR: lib $v not found under .pio/libdeps — build any firmware once (e.g. 'pio run -e WioTrackerL1_companion_radio_usb') to fetch it."; exit 1; }
done

INCLUDES="-I sim/host -I src -I src/helpers -I lib/ed25519 -I $CRYPTO -I $LPP -I $B64"
DEFS="-D HOST_PLATFORM -D MAX_CONTACTS=350 -D MAX_GROUP_CHANNELS=40 -D OFFLINE_QUEUE_SIZE=256 -D MAX_NEIGHBOURS=50"
# -funsigned-char: match the device. ARM defaults plain `char` to unsigned, and
# the UI KEY_* codes (KEY_RIGHT=0xB7 etc.) rely on that. Without it, host signed
# `char` makes those codes negative and all >0x7F key comparisons silently fail.
CXX="clang++ -std=c++17 -O0 -g -funsigned-char -Wall -Wno-unused-parameter -Wno-deprecated-declarations"

# --- shared sources (identical for every profile) ---
HOST="sim/host/arduino.cpp sim/host/crypto_rng_host.cpp sim/host/VirtualRadio.cpp sim/host/target_host.cpp"
CORE="src/Utils.cpp src/Identity.cpp src/Packet.cpp src/Mesh.cpp src/Dispatcher.cpp"
HELPERS="src/helpers/BaseChatMesh.cpp src/helpers/IdentityStore.cpp src/helpers/StaticPoolPacketManager.cpp \
  src/helpers/CommonCLI.cpp src/helpers/ClientACL.cpp src/helpers/AdvertDataHelpers.cpp \
  src/helpers/TxtDataHelpers.cpp src/helpers/RegionMap.cpp src/helpers/TransportKeyStore.cpp \
  src/helpers/ArduinoSerialInterface.cpp"
ED="lib/ed25519/add_scalar.c lib/ed25519/fe.c lib/ed25519/ge.c lib/ed25519/key_exchange.c \
  lib/ed25519/keypair.c lib/ed25519/sc.c lib/ed25519/seed.c lib/ed25519/sign.c lib/ed25519/sha512.c lib/ed25519/verify.c"
CRYPTOSRC="$CRYPTO/AES128.cpp $CRYPTO/AESCommon.cpp $CRYPTO/Crypto.cpp $CRYPTO/BlockCipher.cpp \
  $CRYPTO/Hash.cpp $CRYPTO/SHA256.cpp $CRYPTO/SHA512.cpp $CRYPTO/Ed25519.cpp \
  $CRYPTO/BigNumberUtil.cpp $CRYPTO/Curve25519.cpp $CRYPTO/GF128.cpp"
LPPSRC="$LPP/*.cpp"

# Host-portable UI widget sources: everything in helpers/ui/ except the hardware
# display drivers / buzzer / vibration. Globbed so new widgets (Scrollbar,
# UiFooter, UiIcons, ...) are picked up automatically.
UI_WIDGETS=""
for f in src/helpers/ui/*.cpp; do
  case "$f" in
    *Display.cpp|*OLEDDisplay.cpp|*OLEDDisplayFonts.cpp|*buzzer.cpp|*GenericVibration.cpp) ;;  # hardware: skip
    *) UI_WIDGETS="$UI_WIDGETS $f" ;;
  esac
done

# --- per-profile app sources / include dir / output ---
case "$PROFILE" in
  companion)
    APPINC="-I examples/companion_radio"
    APP="examples/companion_radio/MyMesh.cpp examples/companion_radio/DataStore.cpp sim/host/node_companion.cpp"
    OUT="sim/meshnode-companion" ;;
  companion-ui)
    APPINC="-I examples/companion_radio -I examples/companion_radio/ui-new -I examples/companion_radio/child_mode -I sim"
    DEFS="$DEFS -D UI_HAS_JOYSTICK=1 -D UI_HAS_JOYSTICK_UPDOWN=1 -D CHILD_MODE -D CHILD_DEFAULT_PIN=1234 -D AUTO_OFF_MILLIS=0"
    # Glob the child dirs so new child-mode files are picked up automatically.
    APP="examples/companion_radio/MyMesh.cpp examples/companion_radio/DataStore.cpp \
      examples/companion_radio/ui-new/UITask.cpp \
      examples/companion_radio/child_mode/*.cpp \
      src/helpers/child/*.cpp \
      $UI_WIDGETS \
      sim/host_display.cpp sim/host/node_companion_ui.cpp"
    OUT="sim/meshnode-companion-ui" ;;
  companion-ui-full)
    # Same as companion-ui but WITHOUT child mode: the normal full companion UI.
    APPINC="-I examples/companion_radio -I examples/companion_radio/ui-new -I sim"
    DEFS="$DEFS -D UI_HAS_JOYSTICK=1 -D UI_HAS_JOYSTICK_UPDOWN=1 -D AUTO_OFF_MILLIS=0"
    APP="examples/companion_radio/MyMesh.cpp examples/companion_radio/DataStore.cpp \
      examples/companion_radio/ui-new/UITask.cpp \
      $UI_WIDGETS \
      sim/host_display.cpp sim/host/node_companion_ui.cpp"
    OUT="sim/meshnode-companion-ui-full" ;;
  heltec-v3 | heltec-v4)
    # Single-button device (Heltec V3/V4): SSD1306 128x64, no joystick. The UI
    # uses one button (click=next, double=prev, triple=select, long=enter).
    APPINC="-I examples/companion_radio -I examples/companion_radio/ui-new -I sim"
    DEFS="$DEFS -D PIN_USER_BTN=0 -D AUTO_OFF_MILLIS=0"
    APP="examples/companion_radio/MyMesh.cpp examples/companion_radio/DataStore.cpp \
      examples/companion_radio/ui-new/UITask.cpp \
      $UI_WIDGETS \
      sim/host_display.cpp sim/host/node_companion_ui.cpp"
    OUT="sim/meshnode-$PROFILE" ;;
  panel)
    # Standalone control panel: one peer node on the UDP ether (talks to a
    # separately-running device process). No companion app compiled in.
    APPINC=""
    APP="sim/host/node_panel.cpp"
    OUT="sim/meshnode-panel" ;;
  repeater)
    APPINC="-I examples/simple_repeater"
    APP="examples/simple_repeater/MyMesh.cpp sim/host/node_repeater.cpp"
    OUT="sim/meshnode-repeater" ;;
  min)
    APPINC=""
    APP="sim/host/node_min.cpp"
    OUT="sim/meshnode" ;;
  *)
    echo "unknown profile '$PROFILE' (use: companion | companion-ui | companion-ui-full | heltec-v3 | heltec-v4 | repeater | min)"; exit 1 ;;
esac

# Optional SDL2 graphic-window backend: `sh sim/build-full.sh <profile> sdl`
SDL_FLAGS=""
if [ "$2" = "sdl" ]; then
  [ -d sim/vendor/SDL2.framework ] || { echo "ERROR: SDL2 not vendored — run 'sh sim/get-sdl.sh' first."; exit 1; }
  DEFS="$DEFS -D USE_SDL"
  APP="$APP sim/host/sdl_backend.cpp"
  SDL_FLAGS="-F sim/vendor -framework SDL2 -Wl,-rpath,@executable_path/vendor"
  OUT="$OUT-sdl"
fi

echo "Profile: $PROFILE -> $OUT"
echo "compiling (no output until done; ~20-40s)..."
# shellcheck disable=SC2086
$CXX $INCLUDES $APPINC $DEFS $HOST $CORE $HELPERS $APP $ED $CRYPTOSRC $LPPSRC $SDL_FLAGS -o "$OUT"
echo "built -> $OUT"
