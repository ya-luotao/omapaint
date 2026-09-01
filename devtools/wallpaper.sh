#!/bin/bash
# Paints a wallpaper for the current Omarchy theme (or a theme named as $1)
# with OmaPaint's own tools, sized to the focused monitor.
#
# Usage: devtools/wallpaper.sh [theme-name] [output.png]
# Requires: a built ./build tree; hyprctl+jq for monitor size (optional).
set -euo pipefail

cd "$(dirname "$0")/.."
BUILD_DIR="${BUILD_DIR:-build}"

theme_name=$(echo "${1:-$(omarchy theme current)}" | tr '[:upper:] ' '[:lower:]-')
theme_dir=$(omarchy theme dir "$theme_name")
colors="$theme_dir/colors.toml"
[[ -f "$colors" ]] || { echo "no colors.toml for theme '$theme_name'" >&2; exit 1; }

out="${2:-/tmp/omarchy-wallpaper-$theme_name.png}"

size="3840x2400"
if command -v hyprctl >/dev/null && command -v jq >/dev/null; then
  size=$(hyprctl monitors -j | jq -r '.[] | select(.focused) | "\(.width)x\(.height)"')
fi

cmake --build "$BUILD_DIR" --target draw_wallpaper >/dev/null
"$BUILD_DIR/draw_wallpaper" "$out" "$colors" "$size"
echo "$out"
