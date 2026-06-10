#!/bin/sh
# Build the child-mode UI simulator. Pulls the real Arduino-free UI widgets
# straight from src/ so edits there flow through on rebuild.
set -e
cd "$(dirname "$0")/.."   # repo root

clang++ -std=c++17 -O0 -g -Wall -Wextra \
  -I src \
  -I examples/companion_radio/child_mode \
  sim/host_display.cpp \
  sim/host_input.cpp \
  sim/host_buzzer.cpp \
  sim/main.cpp \
  "src/helpers/ui/MenuModel.cpp" \
  "src/helpers/ui/ListMenuScreen.cpp" \
  "src/helpers/ui/PinEntryScreen.cpp" \
  "src/helpers/ui/StatusHeader.cpp" \
  "src/helpers/child/ChildConfig.cpp" \
  "src/helpers/child/ChildCommands.cpp" \
  "src/helpers/child/ChildMessageStore.cpp" \
  "src/helpers/child/ChildTextWrap.cpp" \
  "examples/companion_radio/child_mode/ChildMessageScreen.cpp" \
  -o sim/meshsim

echo "built -> sim/meshsim   (run: ./sim/meshsim)"
