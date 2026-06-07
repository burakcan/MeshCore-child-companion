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
| p | inject incoming `!pin 1234 5678` | (simulated packet) |
| q | quit | — |

The PIN persists to `sim/child_cfg.bin` (real `ChildConfig` pack/unpack), so
the `!pin` command survives a restart. Delete that file to reseed PIN `1234`.

## What is real vs. mirrored

**Compiled straight from the firmware tree** (your edits flow through on
rebuild — do not duplicate these):
`helpers/ui/{MenuModel,ListMenuScreen,PinEntryScreen,StatusHeader}.cpp`,
`helpers/child/{ChildConfig,ChildCommands}.cpp`.

**Mirrored in `sim/main.cpp`** (because the originals pull in `MyMesh.h` /
`UITask.h` = full firmware): the home screen (`ChildHomeScreen`) and the
screen-flow glue (`ChildMode`'s menu/PIN/incoming-text state machine).
⚠️ If you change the flow in `examples/companion_radio/child_mode/`, mirror it
in `sim/main.cpp`.

## Files

- `host_display.{h,cpp}` — `DisplayDriver` → 1-bit framebuffer → terminal.
- `host_input.{h,cpp}` — raw-terminal keyboard → `KEY_*` codes.
- `main.cpp` — wires the real widgets into the child-mode flow.
- `glcdfont.h` — vendored Adafruit classic 5×7 font (pixel-accurate text).
- `build.sh` — standalone clang++ build.

## Not covered (by design — this is the "thin" harness)

Real `UITask` loop, real `MyMesh`/mesh stack, BLE/serial, sensors, joystick
debounce timing. Those need the full firmware-on-host harness. For child-mode
UI work, this exercises the screens, navigation, PIN flow, and `!pin` parsing.
