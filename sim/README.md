# Child-mode UI simulator (`sim/`)

A PC harness to run and iterate on the MeshCore **child-mode** UI without
flashing a Wio Tracker L1. It renders the firmware's 128×64 display to your
terminal (Unicode half-blocks, real Adafruit font) and maps your keyboard onto
the L1 joystick.

## Build & run

```sh
sh sim/build.sh      # -> sim/meshsim
./sim/meshsim
```

Requires `clang++` (C++17). No PlatformIO, no SDL, no hardware.

## Controls

| Key | Acts as | Firmware key |
|---|---|---|
| ← / → (or h / l) | joystick left / right | `KEY_LEFT` / `KEY_RIGHT` |
| ↑ / ↓ (or k / j) | joystick up / down | `KEY_UP` / `KEY_DOWN` |
| Enter | joystick press | `KEY_ENTER` |
| Backspace / Esc | back | `KEY_CANCEL` |
| s | back-button triple-click | `KEY_SELECT` |
| m | inject an incoming **DM** (pops the reader) | (simulated packet) |
| n | inject an incoming **channel** message | (simulated packet) |
| p | inject incoming `!pin 1234 5678` | (simulated packet) |
| b | mute / unmute the emulated buzzer | |
| q | quit | |

Try: `m` then `n` to receive two messages, back out to home (see the
"New: N" badge), then Enter → "Messages" to browse the list and open the
reader (↑/↓ scroll long messages).

The PIN persists to `sim/child_cfg.bin` (real `ChildConfig` pack/unpack), so
the `!pin` command survives a restart. Delete that file to reseed PIN `1234`.

## What is real vs. mirrored

**Compiled straight from the firmware tree** (your edits flow through on
rebuild — do not duplicate these):
`helpers/ui/{MenuModel,ListMenuScreen,PinEntryScreen,StatusHeader}.cpp`,
`helpers/child/{ChildConfig,ChildCommands,ChildMessageStore,ChildTextWrap}.cpp`,
and the real message reader `child_mode/ChildMessageScreen.cpp`.

**Mirrored in `sim/main.cpp`** (because the originals pull in `MyMesh.h` /
`UITask.h` = full firmware): the home screen (`ChildHomeScreen`, incl. the
unread badge) and the screen-flow glue (`ChildMode`'s menu / PIN / messages-list
/ reader-return / incoming-text state machine).
⚠️ If you change the FLOW or the HOME layout in
`examples/companion_radio/child_mode/{ChildMode,ChildHomeScreen}.cpp`, mirror it
in `sim/main.cpp`. New self-contained screens/stores/helpers (like the message
code) only need adding to `sim/build.sh`.

## Sleep / wake (display auto-off)

Mirrors `UITask`'s `AUTO_OFF_MILLIS` behavior:

- The display blanks after **15 s** of no input (status line shows a live
  `sleep in Ns` countdown, then `[ASLEEP]`).
- Any UI key while asleep **wakes the display and is swallowed** (it doesn't
  also act on the current screen) — exactly like `checkDisplayOn()`.
- An incoming message (`m` / `n`) **wakes** the display (mirrors the CHILD_MODE
  `showScreenAwake()`); the `!pin` command (`p`) does **not** wake it.

15 s is slow to test, so override it:

```sh
SIM_AUTO_OFF_MS=3000 ./sim/meshsim    # sleep after 3s
SIM_AUTO_OFF_MS=0    ./sim/meshsim    # never sleep (like the e-ink variant)
```

## Buzzer (sound)

The emulator plays the **same RTTTL ringtones** the firmware uses (startup,
DM = `MsgRcv3`, channel = `kerplop`, PIN ack = `ack`), with the same note table
and tempo math as the device's `NonBlockingRtttl`. It synthesizes a square wave
(matching the piezo) and plays it via macOS `afplay`; on non-macOS it falls back
to the terminal bell. Press `b` to mute. Note durations match the device to the
millisecond.

⚠️ The ringtone strings live in `sim/main.cpp` (copied from `UITask::notify()` /
`genericBuzzer`). If the firmware changes a tune, mirror it there.

## Files

- `host_display.{h,cpp}` — `DisplayDriver` → 1-bit framebuffer → terminal.
- `host_input.{h,cpp}` — raw-terminal keyboard → `KEY_*` codes.
- `host_buzzer.{h,cpp}` — RTTTL parser → square-wave WAV → `afplay`.
- `main.cpp` — wires the real widgets into the child-mode flow.
- `glcdfont.h` — vendored Adafruit classic 5×7 font (pixel-accurate text).
- `build.sh` — standalone clang++ build.

## Not covered (by design — this is the "thin" harness)

Real `UITask` loop, real `MyMesh`/mesh stack, BLE/serial, sensors, joystick
debounce timing. Those need the full firmware-on-host harness. For child-mode
UI work, this exercises the screens, navigation, PIN flow, `!pin` parsing,
message receive/read, buzzer ringtones, and display sleep/wake.
