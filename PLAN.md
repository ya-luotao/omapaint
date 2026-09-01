# OmaPaint Plan

## Overview

**OmaPaint** is a small, native image editor designed for Omarchy.

The goal is not to compete with Krita, GIMP, or other full-featured graphics applications. OmaPaint aims to fill the same role that classic Paint once filled on Windows:

- open an image,
- draw on it,
- crop it,
- add a label or arrow,
- copy it,
- save it,
- get out of the way.

The product should feel fast, obvious, lightweight, and native to the Omarchy desktop.

> Paint. For Omarchy.

---

## Goals

OmaPaint should be:

- Fast to launch.
- Simple enough to use without documentation.
- Native to the Omarchy visual language.
- Keyboard-friendly without becoming keyboard-only.
- Useful for quick drawing, general image edits, and — optionally — screenshot annotation.
- Small enough that the architecture remains understandable.
- Reliable with ordinary PNG, JPEG, and WebP images.
- A good project for community contributions.

The initial target is a practical everyday utility, not a general-purpose graphics platform.

---

## Non-Goals

OmaPaint will not initially include:

- Layers.
- Vector documents.
- PSD support.
- Complex filters.
- Animation.
- Cloud storage.
- Accounts or synchronization.
- Plugin systems.
- AI image generation.
- Professional color-management workflows.
- Advanced photo-editing tools.
- Its own screenshot capture implementation.

If a feature makes OmaPaint significantly harder to understand without making common image-editing tasks substantially easier, it probably does not belong in the core application.

---

## Prior Art

OmaPaint is not being written into a vacuum, and the plan should be honest about that:

- **tensaku** is Omarchy's default screenshot annotation editor. The screenshot
  pipeline (`omarchy-capture-screenshot`) already hands captured images to a
  pluggable editor via `$OMARCHY_SCREENSHOT_EDITOR`. Screenshot annotation is
  therefore *already covered* on Omarchy. OmaPaint can become an alternative
  editor behind that same hook, but it does not need to own capture or claim
  annotation as its founding purpose.
- **KolourPaint** is essentially classic Paint built on Qt/KDE. It works, but it
  pulls KDE Frameworks and does not follow Omarchy theming.
- **Pinta** and **GNOME Drawing** cover the simple-editor niche on GTK, again
  without Omarchy integration.

What none of these provide is a *small, Omarchy-native, blank-canvas paint
program* that shares the desktop's theme, fonts, and QML technology stack
(Omarchy's shell is Quickshell, so Qt 6 is already installed on every Omarchy
system). That — not screenshot annotation — is the gap OmaPaint fills.

---

# Core Use Cases

## Quick Drawing

Launch OmaPaint and immediately start drawing on a blank canvas.

```bash
omapaint
```

Optional canvas dimensions:

```bash
omapaint --new 1280x720
```

---

## Edit an Existing Image

```bash
omapaint screenshot.png
```

Typical workflow:

```text
Open
→ Crop
→ Draw rectangle
→ Add arrow
→ Add text
→ Save
```

---

## Screenshot Annotation (optional, via the Omarchy hook)

OmaPaint must not implement screenshot capture. Omarchy's screenshot pipeline
already exists and already supports pluggable editors:

```text
omarchy-capture-screenshot
        │  grim / slurp capture
        ▼
$OMARCHY_SCREENSHOT_EDITOR   (default: tensaku-edit)
        │
        ▼
     the editor
```

Integration is therefore a thin wrapper script, packaged with OmaPaint:

```bash
# omapaint-edit <file>
# open <file>, save back to <file> on save, copy result to the clipboard
exec omapaint --annotate "$1"
```

Users who prefer OmaPaint over tensaku opt in with one line:

```bash
export OMARCHY_SCREENSHOT_EDITOR=omapaint-edit
```

`--annotate` means: open the file, and on "done" save in place and copy the
result to the Wayland clipboard — matching what the pipeline expects from
tensaku. No `--screenshot` flag, no capture code, no slurp/grim dependency.

---

## Clipboard Editing

Opening an image directly from the Wayland clipboard should be supported.

```bash
omapaint --clipboard
```

**Wayland caveat:** a Wayland client can only read the clipboard while it has
focus, so the image must be read *after* the window is shown and focused, not
during startup — with `wl-paste` as a CLI-side fallback. This is a known
sharp edge and should be handled (and tested) explicitly.

The application should also support normal Copy, Cut, and Paste operations.

---

# Product Principles

## 1. Keep the Mental Model Small

The core document model should remain:

```text
one window
   ↓
one image
   ↓
one canvas
```

There should be no project browser, document hierarchy, workspace manager, or layer panel.

---

## 2. Launch Fast — and Measure It

OmaPaint is a utility.

Opening it should feel closer to launching a terminal utility than starting a creative suite.

"Fast" must be a measurable target, not a slogan:

> Cold launch to interactive canvas in **under 300 ms** on ordinary Omarchy
> hardware. Tracked from v0.0.1 onward.

Avoid:

- unnecessary background services,
- large application frameworks beyond what is required,
- network initialization,
- startup-time indexing,
- plugin discovery,
- hidden persistent processes,
- runtime-loaded loose `.qml` files (use `qt_add_qml_module` with ahead-of-time
  compilation instead).

---

## 3. Integrate With the Desktop, Don't Duplicate It

Omarchy already has screenshot capture and a default annotation editor.
OmaPaint plugs into those seams (`$OMARCHY_SCREENSHOT_EDITOR`, MIME
associations, "Open With") rather than re-implementing any of them.

The workflows OmaPaint must be genuinely good at are the editor-side ones:

- drawing rectangles and arrows,
- adding short text,
- cropping,
- hiding sensitive information (pixelate),
- copying the result.

These receive at least as much attention as freehand drawing — but capture
belongs to Omarchy, not OmaPaint.

---

## 4. Preserve the Spirit of Classic Paint

OmaPaint may borrow the interaction philosophy of early desktop paint applications:

```text
┌──────────────────────────────────┐
│ File  Edit  View                 │
├────────┬─────────────────────────┤
│        │                         │
│ Tools  │         Canvas          │
│        │                         │
│        │                         │
├────────┴─────────────────────────┤
│ Color Palette                    │
└──────────────────────────────────┘
```

The application should not copy proprietary Microsoft artwork, icons, or visual assets.

The interaction can be nostalgic while the visual design remains distinctly Omarchy.

---

# Proposed Technology

The initial implementation should use:

- **Qt 6**
- **Qt Quick / QML** — for the UI shell only
- **C++** — for the document, canvas rendering, tools, and undo system
- **CMake**

Why QML and not QtWidgets? QtWidgets would be marginally simpler and faster to
start for a tool like this. QML is chosen deliberately anyway, for ecosystem
alignment: Omarchy's shell is Quickshell, so Qt 6 + QML is already installed on
every Omarchy machine and is the stack Omarchy contributors already know. That
is a trade-off, and the startup-time cost it carries is why the launch target
above is measured, and why QML is confined to the interface shell.

**The C++ / QML boundary is an architectural rule, not a preference:**

```text
C++  — Document, QImage, canvas item, tools, undo commands, image I/O, clipboard
QML  — window chrome, toolbar, palette, menus, dialogs, status bar
```

Per-pixel raster work in QML/JavaScript is a known performance trap and would
directly contradict the launch-fast and 4K-responsiveness goals. The canvas is
a C++ `QQuickItem` (or `QQuickPaintedItem`) backed by the document's `QImage`,
uploading only damaged regions as textures. Tools receive pointer events in
C++ and mutate the image through undo commands. QML never touches pixels.

The application should run as its own process.

It should **not** run inside the long-lived Omarchy shell process.

```text
Omarchy / Quickshell
        │
        │ launch
        ▼
     OmaPaint
   separate process
```

An image editor can consume substantial memory or encounter malformed image files. A failure in OmaPaint should never bring down the desktop shell.

---

# Architecture

Keep the first architecture intentionally small.

```text
Application
    │
    ├── Document (C++)
    │     ├── QImage
    │     ├── file path
    │     ├── dirty state
    │     └── undo stack (command-based, from day one)
    │
    ├── CanvasItem (C++ QQuickItem, damage-region texture upload)
    │
    ├── Tools (C++: pencil, brush, shapes, fill, selection, text, …)
    │
    ├── Commands (C++: one undo command type per operation)
    │
    ├── Clipboard (C++)
    │
    ├── Image I/O (C++)
    │
    └── UI shell (QML: toolbar, palette, menus, status bar)
```

Suggested repository structure:

```text
omapaint/
├── CMakeLists.txt
├── README.md
├── PLAN.md
├── LICENSE
│
├── src/
│   ├── main.cpp
│   ├── document.cpp / document.h
│   ├── canvas_item.cpp / canvas_item.h
│   ├── image_io.cpp / image_io.h
│   ├── clipboard.cpp / clipboard.h
│   │
│   ├── tools/
│   │   ├── tool.h              (base interface)
│   │   ├── pencil_tool.cpp
│   │   ├── brush_tool.cpp
│   │   ├── eraser_tool.cpp
│   │   ├── rectangle_tool.cpp
│   │   └── ...
│   │
│   └── commands/
│       ├── command.h           (base interface)
│       ├── draw_command.cpp    (region-based before/after)
│       ├── resize_command.cpp  (full-snapshot)
│       └── ...
│
├── qml/
│   ├── Main.qml
│   ├── ToolBar.qml
│   ├── ColorPalette.qml
│   └── StatusBar.qml
│
├── tests/
│   ├── document/
│   ├── image_io/
│   ├── tools/
│   └── undo/
│
└── packaging/
    └── arch/
        └── (PKGBUILD, omapaint-edit wrapper, .desktop, MIME)
```

Do not introduce a plugin architecture in the initial versions.

---

# Core Tools

The first stable release should eventually contain:

- Pencil
- Brush
- Eraser
- Line
- Rectangle
- Ellipse
- Fill
- Eyedropper
- Rectangular selection
- Text
- Arrow
- Crop

Useful later additions:

- Pixelate
- Blur
- Rounded rectangle
- Simple shape presets

---

# Color Model

The initial UI should provide:

- foreground color,
- background color,
- fixed quick-access palette,
- custom color picker.

A classic visible color palette is preferred over hiding basic color selection behind dialogs.

Transparency should be preserved when editing formats that support it.

---

# Zoom, HiDPI, and Pixel Editing

Supported zoom levels should include approximately:

```text
25%
50%
100%
200%
400%
800%
1600%
```

At high zoom levels, OmaPaint should optionally display a pixel grid.

**HiDPI is a correctness requirement, not polish.** Hyprland users commonly run
fractional scaling. The canvas must maintain an exact mapping between image
pixels and rendered pixels at every zoom level and every `devicePixelRatio` —
the pixel grid, the eyedropper, and single-pixel pencil edits all depend on it.
This mapping should be handled explicitly and covered by tests.

This makes the application useful for very small image and icon edits without turning it into a dedicated pixel-art editor.

---

# Selection

The initial selection system should support:

- rectangular selection,
- move,
- delete,
- cut,
- copy,
- paste,
- crop to selection.

Selection behavior should remain raster-oriented.

**Scope warning:** moving a selection and pasting both imply a *floating
selection* — an uncommitted pixel layer that follows the pointer, commits on
click-outside or tool change, and interacts with the undo stack. This is the
classic hard part of every raster editor and should be budgeted as the main
work item of its milestone, not a bullet point among ten.

Advanced object transforms are outside the initial scope.

---

# Undo and Redo

The undo architecture is part of the **first** milestone, not a later feature.
Drawing tools are implemented *as* undo commands from day one; retrofitting a
command model under existing tools is exactly the rework this plan exists to
avoid.

A naïve implementation that copies the entire image for every mouse movement will become prohibitively expensive with large images.

Prefer command-level history.

For a drawing operation:

```text
pointer down
    ↓
begin operation
    ↓
track affected region
    ↓
pointer up
    ↓
commit undo command
```

Each command can preserve the relevant before/after image region.

Operations such as image resize or crop may store a full image snapshot when necessary.

One continuous brush stroke should normally correspond to one undo action.

---

# Keyboard Shortcuts

OmaPaint should be usable efficiently without requiring users to memorize shortcuts.

Possible defaults:

```text
P        Pencil
B        Brush
E        Eraser
L        Line
R        Rectangle
O        Ellipse
F        Fill
I        Eyedropper
T        Text
S        Selection

[        Smaller brush
]        Larger brush

+        Zoom in
-        Zoom out

Ctrl+N   New
Ctrl+O   Open
Ctrl+S   Save
Ctrl+Shift+S
         Save As

Ctrl+Z   Undo
Ctrl+Shift+Z
         Redo

Ctrl+X   Cut
Ctrl+C   Copy
Ctrl+V   Paste
```

Shortcuts should complement the graphical interface rather than replace it.

---

# Image Formats

Initial support:

- PNG
- JPEG
- WebP

PNG should receive the most testing because it will likely be the default format for screenshots and transparent images.

**Packaging note:** WebP support comes from the `qt6-imageformats` plugin
package, which is *not* part of `qt6-base`. It must be an explicit dependency
in the Arch package, and its absence should degrade gracefully (WebP simply
absent from dialogs) rather than crash.

Additional Qt-supported image formats may work automatically, but they should not be advertised until tested.

---

# Omarchy Integration

OmaPaint should feel like part of Omarchy rather than a generic Qt application installed alongside it.

**Theme integration has a concrete mechanism:** every Omarchy theme ships a
`colors.toml` (under the theme directory, e.g.
`/usr/share/omarchy/themes/<name>/colors.toml`). OmaPaint reads the current
theme's `colors.toml` and maps it onto its QML palette. There is no
Qt-specific theme output in Omarchy, so this mapping is OmaPaint's own code —
a real (if modest) work item, including reacting to theme switches at runtime.

Planned integration:

- Colors from the active theme's `colors.toml`.
- Omarchy fonts.
- Window styling consistent with the desktop.
- Application launcher integration.
- `.desktop` file.
- MIME associations.
- File-manager “Open With OmaPaint”.
- Wayland clipboard integration.
- `omapaint-edit` wrapper for `$OMARCHY_SCREENSHOT_EDITOR`.

Theme integration should fail gracefully: with no Omarchy theme present,
OmaPaint falls back to a sane built-in palette and remains fully usable
outside Omarchy.

---

# CLI

The CLI should remain simple.

Initial commands:

```bash
omapaint
omapaint image.png
omapaint --new 800x600
omapaint --clipboard    # lands in v0.0.4
```

Planned:

```bash
omapaint --annotate image.png   # open, save-in-place + copy on done
                                # (used by the omapaint-edit wrapper)
```

Piping an image into the application may be explored later:

```bash
wl-paste | omapaint -
```

---

# Milestones

## v0.0.1 — Paint Something

Goal: prove the application architecture — including the parts that are
expensive to retrofit: the C++ canvas item and the undo command stack.

Features:

- Application window (QML shell).
- C++ canvas item with damage-region rendering.
- New document.
- Pencil.
- Eraser.
- Foreground color.
- Open PNG.
- Save PNG.
- Dirty-state detection.
- **Undo/redo command stack** (even if pencil and eraser are the only command
  types yet).
- Basic keyboard shortcuts.
- Startup-time measurement in place (< 300 ms target).

Success criterion:

> A user can launch OmaPaint, draw an image, undo and redo strokes, save it,
> reopen it, and continue editing.

---

## v0.0.2 — Basic Paint

Features:

- Brush.
- Line.
- Rectangle.
- Ellipse.
- Fill.
- Eyedropper.
- Brush size.
- Color palette.
- Undo coverage extended to every new tool.
- Zoom.
- Pixel grid (pixel-exact under fractional scaling).

Success criterion:

> OmaPaint is usable as a small general-purpose paint program.

---

## v0.0.3 — Edit Images

**Main work item: the floating-selection state machine** (uncommitted pixel
layer, commit on click-outside/tool-change, undo interaction). Everything else
in this milestone is small by comparison; budget accordingly.

Features:

- Rectangular selection.
- Move selection (floating).
- Cut.
- Copy.
- Paste (as floating selection).
- Crop.
- Resize image.
- Resize canvas.
- JPEG support.
- WebP support (via `qt6-imageformats`).

Success criterion:

> Existing screenshots and images can be edited without requiring another application.

---

## v0.0.4 — Annotate

**Main work item: the text tool's editing state** (editable-before-commit text
box, font selection, input method support). Like the floating selection, this
is a state machine, not a drawing primitive.

Features:

- Text.
- Arrow.
- Filled and outlined shapes.
- Better stroke controls.
- Clipboard image launch (`--clipboard`, with the Wayland focus caveat handled).
- Optional pixelate tool.

Success criterion:

> OmaPaint is a credible editor for everyday screenshot annotation.

---

## v0.1.0 — Omarchy Native

Features:

- Theme integration via `colors.toml` (with runtime theme-switch handling and
  graceful fallback).
- Application launcher integration.
- MIME integration.
- Arch packaging (including `qt6-imageformats` dependency).
- `--annotate` mode and the `omapaint-edit` wrapper for
  `$OMARCHY_SCREENSHOT_EDITOR`.
- UI polish.
- Improved error handling.
- Performance testing.
- Stable basic file handling.

Success criterion:

> OmaPaint is ready to be recommended as a normal Omarchy utility — and users
> who prefer it can make it their screenshot editor with one environment
> variable.

---

# Testing

Image editors are easy to test manually and surprisingly easy to break.

Automated testing should be added alongside features. Because the undo stack
exists from v0.0.1, undo invariants are testable from the very first release.

Important areas:

### Image I/O

- Save/load round trips.
- Alpha transparency.
- JPEG conversion.
- Invalid files.
- Very small images.
- Large images.
- Unsupported formats.

### Drawing

Given known input coordinates, verify expected pixels for:

- lines,
- rectangles,
- ellipses,
- pencil strokes,
- fill operations,
- erasing.

### Undo / Redo

Every editing operation should satisfy:

```text
original
   ↓
operation
   ↓
undo
   ↓
original
```

and:

```text
operation
   ↓
undo
   ↓
redo
   ↓
same result as operation
```

### Clipboard

Test:

- copying a selection,
- copying a full image,
- pasting images,
- invalid clipboard content,
- clipboard transparency,
- `--clipboard` startup behavior under Wayland focus rules.

### HiDPI / Zoom

- Pixel-exact image-to-screen mapping at every zoom level.
- Eyedropper accuracy under fractional scaling.
- Pixel grid alignment at high zoom.

### Performance

Smoke-test:

- 4K screenshots,
- large brush strokes,
- repeated undo/redo,
- high zoom,
- large pasted images,
- cold-start time against the 300 ms target.

The goal is not professional graphics performance, but ordinary desktop images should remain responsive.

---

# Community Contribution Model

OmaPaint should be easy to contribute to.

Good first contributions may include:

- additional simple shapes,
- keyboard shortcuts,
- palette improvements,
- icon work,
- file-format tests,
- accessibility improvements,
- packaging,
- documentation,
- Wayland integration tests.

Suggested issue labels:

```text
good-first-issue
drawing
image-io
wayland
omarchy-integration
design
performance
testing
accessibility
```

Large architectural changes should be discussed before implementation.

---

# Design Constraints

When reviewing a feature, ask:

### Does this make a common task significantly easier?

If not, it may not belong.

### Does this require introducing a new abstraction?

If yes, make sure the feature justifies it.

### Does Omarchy already provide this?

If yes, integrate with it instead of re-implementing it (screenshots, theming,
launchers).

### Would a user expect this from Paint?

If yes, it is probably worth considering.

### Would a user expect this from Photoshop or Krita?

If yes, it probably belongs somewhere else.

---

# Possible Future Work

These ideas are intentionally outside the first milestone sequence but may be explored later:

- Simple command palette.
- Tablet/stylus pressure.
- Better touch support.
- Configurable palettes.
- Image rotation and flipping.
- Basic EXIF orientation handling.
- Drag-and-drop.
- Optional autosave/recovery.
- Small pixel-art conveniences.
- Portal-based desktop integration.
- `wl-paste | omapaint -` stdin support.

None of these should delay the core editor.

---

# Definition of Success

OmaPaint succeeds if an Omarchy user can think:

> “I just need to quickly edit this image.”

and instinctively open OmaPaint.

It should take seconds to:

```text
open
draw
annotate
crop
copy
save
```

without turning a simple task into a graphics project.

The long-term goal is not to make OmaPaint powerful.

The goal is to make it **useful enough that nobody misses Paint**.
