#!/bin/sh
# Vendor SDL2 into the project (no global install). Downloads the official
# SDL2.framework into sim/vendor/ (gitignored, found at runtime via rpath).
set -e
cd "$(dirname "$0")"
VENDOR="vendor"
mkdir -p "$VENDOR"

if [ -d "$VENDOR/SDL2.framework" ]; then
  echo "SDL2.framework already vendored at sim/$VENDOR/ — nothing to do."
  exit 0
fi

VER="${SDL2_VERSION:-2.30.9}"
DMG="/tmp/SDL2-$VER.dmg"
URL="https://github.com/libsdl-org/SDL/releases/download/release-$VER/SDL2-$VER.dmg"

echo "downloading SDL2 $VER ..."
curl -fL "$URL" -o "$DMG"

MNT="$(mktemp -d)"
echo "mounting + extracting SDL2.framework ..."
hdiutil attach "$DMG" -mountpoint "$MNT" -nobrowse -quiet
cp -R "$MNT/SDL2.framework" "$VENDOR/"
hdiutil detach "$MNT" -quiet
rm -f "$DMG"

echo "vendored -> sim/$VENDOR/SDL2.framework  (run a build with the 'sdl' option to use it)"
