#!/bin/bash
# Regenerates assets/readme-preview.png: draws the Omarchy logo with
# OmaPaint's tool engine (in the current Omarchy theme's colors), opens it
# in OmaPaint, floats the window, and screenshots it.
#
# Requires: a built ./build tree, a running Omarchy/Hyprland session, grim, jq.
set -euo pipefail

cd "$(dirname "$0")/.."
BUILD_DIR="${BUILD_DIR:-build}"
OUT="assets/readme-preview.png"
TMP_PNG="$(mktemp --suffix=.png)"
trap 'rm -f "$TMP_PNG"' EXIT

# Current Omarchy theme colors, with graceful fallback.
BG="white"
FG="black"
if command -v omarchy-theme-current >/dev/null; then
  # `theme current` prints a display name ("Kanagawa"); dirs are kebab-case.
  theme_name=$(omarchy theme current | tr '[:upper:] ' '[:lower:]-')
  theme_dir=$(omarchy theme dir "$theme_name" 2>/dev/null || true)
  if [[ -f "$theme_dir/colors.toml" ]]; then
    BG=$(awk -F'"' '/^background/ { print $2; exit }' "$theme_dir/colors.toml")
    FG=$(awk -F'"' '/^foreground/ { print $2; exit }' "$theme_dir/colors.toml")
  fi
fi

cmake --build "$BUILD_DIR" --target draw_logo omapaint
"$BUILD_DIR/draw_logo" "$TMP_PNG" "$BG" "$FG"

"$BUILD_DIR/omapaint" "$TMP_PNG" &
APP_PID=$!
trap 'kill $APP_PID 2>/dev/null; rm -f "$TMP_PNG"' EXIT
sleep 1.5

# Float and size the window so the whole canvas is visible (Omarchy's
# Hyprland exposes Lua dispatchers through hyprctl).
hyprctl dispatch "hl.dsp.focus({ window = 'class:omapaint' })" >/dev/null
hyprctl dispatch "hl.dsp.window.float({ action = 'on' })" >/dev/null
hyprctl dispatch "hl.dsp.window.resize({ x = 1500, y = 920 })" >/dev/null

# Center it on the focused monitor (coordinates are global logical pixels).
read -r MX MY MW MH MSCALE < <(hyprctl monitors -j |
  jq -r '.[] | select(.focused) | "\(.x) \(.y) \(.width) \(.height) \(.scale)"')
LW=$(awk "BEGIN { printf \"%d\", $MW / $MSCALE }")
LH=$(awk "BEGIN { printf \"%d\", $MH / $MSCALE }")
hyprctl dispatch "hl.dsp.window.move({ x = $((MX + (LW - 1500) / 2)), y = $((MY + (LH - 920) / 2)) })" >/dev/null
sleep 0.8

GEOM=$(hyprctl clients -j |
  jq -r '.[] | select(.class=="omapaint") | "\(.at[0]),\(.at[1]) \(.size[0])x\(.size[1])"' | head -1)
mkdir -p assets
grim -g "$GEOM" "$OUT"
echo "wrote $OUT"
