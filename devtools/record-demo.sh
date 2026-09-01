#!/bin/bash
# Records the self-driving promo demo (`omapaint --demo`) with
# gpu-screen-recorder. Requires a live Omarchy/Hyprland session.
#
#   devtools/record-demo.sh [output.mp4]
set -euo pipefail

cd "$(dirname "$0")/.."
BUILD_DIR="${BUILD_DIR:-build}"
OUT="${1:-/tmp/omapaint-demo.mp4}"

ORIG_THEME=$(cat ~/.local/state/omarchy/current/theme.name 2>/dev/null || true)
ORIG_WS=$(hyprctl activeworkspace -j | jq -r '.id')

# Record on an empty workspace so nothing else is in frame.
USED=$(hyprctl workspaces -j | jq -r '.[] | select(.windows > 0) | .id')
DEMO_WS=""
for i in $(seq 1 10); do
  if [[ $i != "$ORIG_WS" ]] && ! grep -qx "$i" <<<"$USED"; then
    DEMO_WS=$i
    break
  fi
done
[[ -z $DEMO_WS ]] && DEMO_WS=10

cmake --build "$BUILD_DIR" --target omapaint

hyprctl dispatch "hl.dsp.focus({ workspace = '$DEMO_WS' })" >/dev/null
sleep 0.4

"$BUILD_DIR/omapaint" --demo &
APP=$!
cleanup() {
  kill "$APP" 2>/dev/null || true
  # The demo switches themes; make sure the desktop is back where it was.
  [[ -n $ORIG_THEME ]] && omarchy-theme-set "$ORIG_THEME" >/dev/null 2>&1 || true
  hyprctl dispatch "hl.dsp.focus({ workspace = '$ORIG_WS' })" >/dev/null 2>&1 || true
}
trap cleanup EXIT
sleep 2

hyprctl dispatch "hl.dsp.focus({ window = 'class:omapaint' })" >/dev/null
hyprctl dispatch "hl.dsp.window.float({ action = 'on' })" >/dev/null
hyprctl dispatch "hl.dsp.window.resize({ x = 1500, y = 920 })" >/dev/null

read -r MX MY MW MH MSCALE < <(hyprctl monitors -j |
  jq -r '.[] | select(.focused) | "\(.x) \(.y) \(.width) \(.height) \(.scale)"')
LW=$(awk "BEGIN { printf \"%d\", $MW / $MSCALE }")
LH=$(awk "BEGIN { printf \"%d\", $MH / $MSCALE }")
hyprctl dispatch "hl.dsp.window.move({ x = $((MX + (LW - 1500) / 2)), y = $((MY + (LH - 920) / 2)) })" >/dev/null
sleep 0.5

read -r X Y W H < <(hyprctl clients -j |
  jq -r '.[] | select(.class=="omapaint") | "\(.at[0]) \(.at[1]) \(.size[0]) \(.size[1])"' | head -1)

gpu-screen-recorder -w region -region "${W}x${H}+${X}+${Y}" \
  -f 60 -cursor no -q very_high -k h264 -o "$OUT" &
REC=$!

START=$SECONDS
wait "$APP" || true
sleep 0.6
kill -INT "$REC" 2>/dev/null || true
wait "$REC" 2>/dev/null || true

echo "demo ran $((SECONDS - START))s, wrote $OUT"
