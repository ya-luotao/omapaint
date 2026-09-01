# OmaPaint — Agent Guide

Small native image editor for Omarchy (Arch + Hyprland). Qt 6 / QML shell,
C++ core, CMake. Read [PLAN.md](PLAN.md) for goals, non-goals, and the
milestone roadmap before proposing features — the scope discipline there is
load-bearing.

## Build, test, run

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure   # all suites, offscreen
./build/omapaint [file.png|--new WxH|--clipboard|--annotate file]
```

Headless smoke test (checks QML loads cleanly + startup time budget <300ms):

```bash
QT_FORCE_STDERR_LOGGING=1 QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software \
  timeout 3 ./build/omapaint    # expect "engine loaded"/"first frame" lines
```

## Architecture rules (enforced, not preferences)

- **QML never touches pixels.** Canvas rendering, tools, undo commands,
  image I/O, selection, clipboard are C++ (`src/`). QML (`qml/`) is the
  interface shell only: toolbar, palette, dialogs, the text-edit overlay.
- **One user gesture = one undo command.** Tools paint directly into the
  document image and report damage rects; `CanvasItem` snapshots the image
  on press (cheap — QImage is copy-on-write), crops before/after to the
  accumulated damage on release, and pushes one region-based `DrawCommand`.
  Size-changing operations (crop/resize) use full-snapshot `ImageCommand`.
- **CanvasItem is viewport-sized**, never `image size × zoom` — a
  QQuickPaintedItem allocates a texture of its item size, so a zoomed item
  would explode memory (1280×720 at 1600% ≈ 900 MB). The Flickable only
  supplies scrollbars and pan state; paint() translates/scales.
- **undo/redo route through `canvas.undo()/redo()`**, never `doc.undo()`
  directly from QML — a pending floating selection must be cancelled or
  committed first. Same idea: call `commitPendingEdits()` before save/new/
  open/resize.
- **Single-letter shortcuts are guarded by `window.typing`** — any new text
  input surface must be added to that property or typing will switch tools.

## Verification recipes (this machine: Omarchy / Hyprland / Wayland)

Float and position the app window (this Hyprland exposes Lua dispatchers):

```bash
hyprctl dispatch "hl.dsp.focus({ window = 'class:omapaint' })"
hyprctl dispatch "hl.dsp.window.float({ action = 'on' })"
hyprctl dispatch "hl.dsp.window.resize({ x = 1500, y = 920 })"
hyprctl dispatch "hl.dsp.window.move({ x = 786, y = 1868 })"  # monitor origin is NOT 0,0 — check hyprctl monitors -j
```

Screenshot for visual checks:

```bash
geom=$(hyprctl clients -j | jq -r '.[] | select(.class=="omapaint") | "\(.at[0]),\(.at[1]) \(.size[0])x\(.size[1])"')
grim -g "$geom" /tmp/shot.png
```

- Keyboard synthesis: `wtype` (e.g. `wtype -M ctrl v -m ctrl`). There is NO
  pointer synthesis on this machine — for scripted mouse interaction use
  `omapaint --demo` (in-app event injection, `src/demo_driver.*`).
- Clipboard: `wl-copy -t image/png < f.png`, verify with
  `wl-paste --list-types`.
- Full screenshot-hook chain: `PATH=$PWD/build:$PATH packaging/omapaint-edit f.png`,
  press Ctrl+Enter (Done), expect exit 0 and image/png on the clipboard.

## Known traps (each one cost real debugging time)

- **`pkill -f build/omapaint` kills your own shell** — the compound command's
  own cmdline contains the pattern. Run `pkill -f '[b]uild/omapaint'` as its
  own separate command, nothing else in the same shell invocation.
- **`QImageWriter::format()` is empty when constructed with only a file
  name** — resolve the format from the suffix yourself (see `image_io.cpp`;
  this silently forced every save to PNG once).
- **Wayland clipboard needs focus** — read it after the window is active
  (`--clipboard` defers to `onActiveChanged`), and a client's clipboard
  vanishes when it exits (that is why `omapaint-edit` does the `wl-copy`,
  not the app).
- **Zero-length lines don't stroke with wide pens** — single-dot input draws
  via `drawPoint` (see `Tool::drawSegment`).
- **Omarchy theme switches rebuild the staged theme dir** — file watchers
  lose the path; `Theme::rewatch()` re-adds after every change. Current theme
  colors live at `~/.local/state/omarchy/current/theme/colors.toml`.
- **Half-applying a theme is worse than none** — if `colors.toml` lacks
  `background`/`foreground`, leave the system palette completely alone.

## Devtools (not part of the product)

- `draw_logo` — paints the README preview image with OmaPaint's own tools.
- `draw_icons` — regenerates the committed tool icons + app logo
  (`assets/`); icons are black ink, tinted to the theme at runtime.
- `draw_wallpaper` + `devtools/wallpaper.sh` — paints an Omarchy wallpaper
  in any theme's colors with OmaPaint's tools (community fun, monitor-sized);
  `devtools/wallpaper-gallery.sh` regenerates `assets/wallpaper-gallery.png`
  (one tile per installed theme) for the README.
- `devtools/readme-preview.sh` — regenerates `assets/readme-preview.png`
  (draws logo in current theme colors, opens app, floats, screenshots).
- `omapaint --demo` + `devtools/record-demo.sh` — scripted self-driving demo
  for promo recordings (gpu-screen-recorder); `src/demo_driver.*` injects
  QMouseEvent/QKeyEvent/QWheelEvent into the real window.

## Conventions

- Tests accompany features; suites live under `tests/<area>/`. Undo
  invariants (`op→undo→original`, `undo→redo→same`) are mandatory for any
  new editing operation. Run everything with `QT_QPA_PLATFORM=offscreen`.
- Commit messages describe the milestone/feature and its verification.
- Do not add: layers, plugins, vector objects, filters — see PLAN.md
  non-goals. When in doubt: "Would classic Paint have it?"
