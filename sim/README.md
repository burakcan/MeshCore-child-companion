# MeshCore PC simulator (`sim/`)

Run MeshCore firmware on your computer, no board, no flashing. Two harnesses,
both rendering the device display to your terminal (Unicode half-blocks, real
Adafruit font) and mapping the keyboard onto the joystick:

| Harness | Build | Use it for |
|---|---|---|
| **Thin UI sim** | `sh sim/build.sh` → `sim/meshsim` | fast iteration on child-mode **screens** only. Compiles a handful of UI widgets; mocks everything else. Seconds to build. |
| **Full firmware-on-host** | `sh sim/build-full.sh <profile>` | the real firmware (real `MyMesh`/`Dispatcher`/crypto/`UITask`) with a virtual radio. Heavier; this is where "see what the device transmits" and the real UI live. |

**Terminal width.** The 128px display renders as 130 columns (1 char/pixel). In a
narrower window (e.g. an iTerm split pane) it auto-switches to a compact 66-column
mode (2×2 pixels per char); below ~66 cols it asks you to widen. Force a mode with
`SIM_DISPLAY=half` or `SIM_DISPLAY=quad`. Resizing adapts automatically.

**Real graphic window (SDL2).** For a resizable window with no terminal limits:
```sh
sh sim/get-sdl.sh                              # one-time: vendors SDL2.framework into sim/vendor/ (no brew)
sh sim/build-full.sh companion-ui-full sdl     # build the SDL variant -> *-sdl
SIM_DISPLAY=sdl ./sim/meshnode-companion-ui-full-sdl
```
SDL2 is vendored into `sim/vendor/` (gitignored) and found at runtime via an
embedded rpath, so nothing is installed globally. Same keys; Esc/`q`/close to quit.
Default builds (no `sdl` arg) don't depend on SDL at all.

Both require `clang++` (C++17). The full harness also needs the PlatformIO libs
(Crypto / CayenneLPP / base64); they're already under `.pio/libdeps/` if you've
built any firmware; otherwise run `pio run -e WioTrackerL1_companion_radio_usb`
once to fetch them.

**Screenshots (`SIM_SCRIPT`).** The `companion-ui` harness can drive itself and
dump each screen to a styled BMP, for the README/docs. It injects mock messages
through the *real* child capture path and presses the *real* buttons, then writes
`$SIM_SHOT_DIR/shot-NN.bmp` (default `sim/shots/`, gitignored):

```sh
sh sim/build-full.sh companion-ui
mkdir -p sim/shots
# script chars: . capture  , settle  c/d/u select·down·up  L/R/b left·right·back
#               M/C dm·channel msg   Q question   B bell   X inbox   m menu
SIM_SCRIPT='.,M,.,c,.' SIM_SHOT_DIR=sim/shots ./sim/meshnode-companion-ui
sips -s format png sim/shots/shot-00.bmp --out home.png   # BMP -> PNG (macOS built-in)
```

Inert unless `SIM_SCRIPT` is set, so normal interactive runs are unaffected.

---

# Full firmware-on-host harness (`build-full.sh`)

```sh
sh sim/build-full.sh <profile>     # builds sim/meshnode-<profile>
./sim/meshnode-<profile>
```

## Running different devices / roles / UIs

The harness is **profile-driven**. Because every board's hardware is virtualized
identically (`HostBoard` / `VirtualRadio` / `HostFS` / host clocks), the physical
board (Heltec vs RAK vs L1) is irrelevant; a "device" is just a profile =
app role + UI + build flags.

| Profile | Command | Runs | Interactive? |
|---|---|---|---|
| `min` | `sh sim/build-full.sh min` | minimal real node, sends one advert | no |
| `repeater` | `sh sim/build-full.sh repeater` | real `simple_repeater` firmware | no (headless) |
| `companion` | `sh sim/build-full.sh companion` | real `companion_radio` (headless) | no |
| `companion-ui` | `sh sim/build-full.sh companion-ui` | real companion **+ real ui-new UITask + child mode** | **yes** |
| `companion-ui-full` | `sh sim/build-full.sh companion-ui-full` | real companion **+ real ui-new UITask, NO child mode** (the normal companion UI) | **yes** |
| `heltec-v3` / `heltec-v4` | `sh sim/build-full.sh heltec-v3` | companion UI on a **single-button** device (no joystick) | **yes** |
| `panel` | `sh sim/build-full.sh panel` | **control panel**: device + a peer node; add contacts, send inbound messages, monitor outgoing | **yes (REPL)** |

### Controls: joystick devices (`companion-ui`, `companion-ui-full`)

| Key | Joystick action |
|---|---|
| Enter | center press (`KEY_ENTER`) |
| ← / → (or `a` / `d`) | joystick left / right (`KEY_LEFT` / `KEY_RIGHT`) |
| ↑ / ↓ | joystick up / down (`KEY_UP` / `KEY_DOWN`) |
| `b` | back button |
| `q` | quit |

> **4-way joystick:** stock ui-new only reads left/right + press + back. These
> profiles enable up/down via `-D UI_HAS_JOYSTICK_UPDOWN`, an opt-in block in
> `UITask.cpp` that reads `joystick_up`/`joystick_down` → `KEY_UP`/`KEY_DOWN`.
> To use it on a real L1: define `UI_HAS_JOYSTICK_UPDOWN` and add the two buttons
> to the variant's `target.cpp` (pins 25 / 26). Off by default, so no device change.

### Controls: single-button devices (`heltec-v3` / `heltec-v4`)

The real device navigates with **one** button via multi-click:

| Key | Action | Firmware |
|---|---|---|
| Enter (tap) | click, next | `KEY_NEXT` |
| Enter Enter (double-tap) | previous | `KEY_PREV` |
| Enter ×3 (triple-tap) | select | `KEY_SELECT` |
| `l` | long press, enter/confirm | `KEY_ENTER` |
| `q` | quit | |

> Double/triple-click is timing-based (taps within ~280 ms). Works interactively;
> piping rapid `\r\r` may merge into one press.

`companion-ui` boots into the child home (real `ChildHomeScreen`); `companion-ui-full`
boots the normal companion UI (splash → home → contacts/messages). Both use the
same controls: keys drive real `MomentaryButton`s via a host `digitalRead`
bridge into the real `UITask` joystick handler. They share one host main
(`node_companion_ui.cpp`); child mode is compiled in only when `-D CHILD_MODE` is
set (the `companion-ui` profile), so `companion-ui-full` is also immune to any
in-progress breakage in the child-mode sources.

### Control panel (`panel` profile): two terminals

The panel is a separate process that talks to a running UI device over a UDP
multicast "ether" (real mesh packets, not fakes). Run them side by side:

```sh
# terminal 1: the device, with its screen
./sim/meshnode-companion-ui-full      # (or companion-ui / heltec-v3)

# terminal 2: the control panel
./sim/meshnode-panel
```

Both join `239.71.71.71:7337` and form a 2-node mesh. The UI device adverts once
on boot (and on-demand from its advert page); have the panel `advert` so the two
discover each other, then drive it from the panel:

```
advert            announce ourself, the device adds us as a contact
send <text>       send a DM to the device (its screen shows the incoming message)
contacts          show the device we've discovered
help / quit
```

You'll see the device's screen (terminal 1) react to messages/contacts, and the
panel (terminal 2) print discovery / ACK / reply events. Any UI profile
(`companion-ui`, `companion-ui-full`, `heltec-v3`) joins the ether automatically.

**Adverts / discovery.** Like real MeshCore devices, the UI device adverts on
boot + on-demand, *not* on a timer. To advert again later, use the device's UI
**advert page** (navigate with ←/→ to it, press Enter), or restart. The panel
adverts once on startup and on the `advert` command, so if the device booted
first, just run `advert` from the panel to make them discover each other.

For out-of-range / multi-hop topologies, run the `repeater` profile on the same
ether too; it re-floods adverts and relays packets like a real repeater.

### The TX log

Every packet the firmware transmits is captured at the `Radio` seam and decoded:

```
>>> TX  111 bytes  route=FLOOD  type=ADVERT(0x4)
    raw: 1100 36965b0b ...[Ed25519 sig]... 36965B0B
```

This is real, signed, on-wire-accurate output (real crypto), UI-independent, so
it shows everything any role/feature sends.

## Adding a new device / role

Edit the `case "$PROFILE"` block in `sim/build-full.sh`. A profile sets three
things: include dir (`APPINC`), source list (`APP`), output name (`OUT`), and
optionally extra `DEFS`. Examples:

- **`room_server`**: copy the `repeater` case, point `APPINC`/`APP` at
  `examples/simple_room_server/`. (Its `MyMesh.cpp` may need the same additive
  `HOST_PLATFORM` `#error` guards the repeater needed, see below.)
- **Different screen geometry** (e-ink 250×122, TFT 320×240): `HostDisplay` is
  currently fixed at 128×64. Make its width/height configurable and pass the
  target size in the profile; the UI reads `display.width()/height()`.
- **Different UI** (`ui-tiny` instead of `ui-new`): swap the UITask source +
  include dir + UI flags in a new profile.

## Architecture (host shims, in `sim/host/`)

| Real hardware | Host stand-in |
|---|---|
| LoRa radio (RadioLib) | `VirtualRadio`: captures/decodes TX, injects RX |
| board (nRF52/ESP32 SDK) | `HostBoard` (4 methods) |
| flash (LittleFS/SPIFFS) | `HostFS` → real files under `sim/fsroot/` |
| `<Arduino.h>`, clocks, RNG | `Arduino.h` shim + `arduino.cpp` (real `millis`, etc.) |
| crypto | the real rweather Crypto + `lib/ed25519` (packets are genuine) |
| `<target.h>` (per board) | `sim/host/target.h`: swaps the globals to host types |

Everything else (core `Dispatcher`/`Mesh`/`Identity`, `MyMesh`, `DataStore`,
`UITask`, child mode) is the real firmware, compiled from the tree, so edits in
src/ and the firmware flow through on rebuild.

## Firmware edits this harness required

All additive, guarded by `#if defined(HOST_PLATFORM)`, behavior-neutral for real
device builds:

- `src/helpers/IdentityStore.h`: `FILESYSTEM = HostFS` branch
- `examples/companion_radio/DataStore.cpp`: `format()` host branch
- `examples/simple_repeater/MyMesh.cpp`: `formatFileSystem()` + `saveIdentity()` host branches

---

# Thin UI sim (`build.sh`)

```sh
sh sim/build.sh      # -> sim/meshsim
./sim/meshsim
```

Fast child-mode screen iteration. Compiles only the Arduino-free UI widgets +
child logic; the screen-flow glue and home layout are mirrored in
`sim/main.cpp` (see "real vs mirrored" below).

## Controls

| Key | Acts as | Firmware key |
|---|---|---|
| ← / → (or h / l) | joystick left / right | `KEY_LEFT` / `KEY_RIGHT` |
| ↑ / ↓ (or k / j) | joystick up / down | `KEY_UP` / `KEY_DOWN` |
| Enter | joystick press | `KEY_ENTER` |
| Backspace / Esc | back | `KEY_CANCEL` |
| s | back-button triple-click | `KEY_SELECT` |
| m | inject an incoming **DM** | (simulated packet) |
| n | inject an incoming **channel** message | (simulated packet) |
| p | inject incoming `!pin 1234 5678` | (simulated packet) |
| b | mute / unmute the emulated buzzer | |
| q | quit | |

The PIN persists to `sim/child_cfg.bin`. Delete it to reseed PIN `1234`.

## Real vs. mirrored (thin sim)

- **Real (compiled from the tree):** `helpers/ui/{MenuModel,ListMenuScreen,
  PinEntryScreen,StatusHeader}.cpp`, `helpers/child/{ChildConfig,ChildCommands,
  ChildMessageStore,ChildTextWrap}.cpp`, `child_mode/ChildMessageScreen.cpp`.
- **Mirrored in `sim/main.cpp`** (originals pull in full firmware): the home
  screen + the `ChildMode` flow. If you change the flow or home layout in
  `child_mode/{ChildMode,ChildHomeScreen}.cpp`, mirror it in `sim/main.cpp`. New
  self-contained widgets only need adding to `sim/build.sh`.
  (The full harness has none of this caveat; it runs the real files.)

## Sleep / wake & buzzer (thin sim)

- **Auto-off:** display blanks after 15 s idle (`SIM_AUTO_OFF_MS=3000` to test
  faster; `=0` to disable). A key while asleep wakes + is swallowed; incoming
  messages wake it, `!pin` does not.
- **Buzzer:** plays the real RTTTL ringtones (`MsgRcv3`/`kerplop`/`ack`) as
  square-wave audio via macOS `afplay` (`b` to mute). Ringtone strings live in
  `sim/main.cpp`; mirror firmware changes.

---

# Files

**Thin sim:** `host_display.{h,cpp}`, `host_input.{h,cpp}`, `host_buzzer.{h,cpp}`,
`main.cpp`, `glcdfont.h`, `build.sh`.

**Full harness (`sim/host/`):** `Arduino.h` + `arduino.cpp`, `Print.h`,
`Stream.h`, `HostBoard.h`, `VirtualRadio.{h,cpp}`, `HostFS.h`, `RTClib.h`,
`target.h` + `target_host.cpp`, `crypto_rng_host.cpp`, and the per-profile mains
(`node_min.cpp`, `node_repeater.cpp`, `node_companion.cpp`,
`node_companion_ui.cpp`). Built by `build-full.sh`.

Build/run artifacts you may want to gitignore: `sim/meshsim`, `sim/meshnode*`,
`sim/*.dSYM/`, `sim/child_cfg.bin`, `sim/fsroot/`.
