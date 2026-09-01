#!/bin/bash
# Regenerates assets/wallpaper-gallery.png: paints a wallpaper for every
# installed Omarchy theme with OmaPaint's tools (draw_wallpaper) and tiles
# them into one contact sheet.
#
# Requires: a built ./build tree, the omarchy CLI, ImageMagick (montage).
set -euo pipefail

cd "$(dirname "$0")/.."
BUILD_DIR="${BUILD_DIR:-build}"
OUT="assets/wallpaper-gallery.png"
TMP_DIR=$(mktemp -d)
trap 'rm -rf "$TMP_DIR"' EXIT

cmake --build "$BUILD_DIR" --target draw_wallpaper >/dev/null

n=0
while read -r name; do
  slug=$(echo "$name" | tr '[:upper:] ' '[:lower:]-')
  theme_dir=$(omarchy theme dir "$slug" 2>/dev/null) || continue
  [[ -f "$theme_dir/colors.toml" ]] || continue
  if "$BUILD_DIR/draw_wallpaper" "$TMP_DIR/$slug.png" "$theme_dir/colors.toml" 960x600; then
    n=$((n + 1))
  else
    echo "skipped $name (incomplete colors.toml)" >&2
  fi
done < <(omarchy theme list)

[[ $n -gt 0 ]] || { echo "no themes rendered" >&2; exit 1; }

montage "$TMP_DIR"/*.png -tile 4x -geometry 480x300+4+4 -background none "$OUT"
echo "wrote $OUT ($n themes)"
