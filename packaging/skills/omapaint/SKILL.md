---
name: omapaint
description: >
  Drive OmaPaint, Omarchy's native image editor, from the command line. Use
  when the user wants to annotate a screenshot, open/edit/crop/draw on an
  image, paint on a blank canvas, or edit the image on the clipboard.
  Triggers: annotate, screenshot editor, image editor, paint, draw, crop,
  arrow, label, clipboard image, omapaint.
---

# OmaPaint

OmaPaint is a small native GUI image editor (classic-Paint style) that
follows the active Omarchy theme. It is interactive by design: an agent's
job is to **launch it with the right input and read the result**, not to
draw with it.

## Launching

```bash
omapaint file.png            # edit an existing image (PNG, JPEG, WebP)
omapaint --new 1280x720      # blank canvas of a given size
omapaint --clipboard         # edit the image on the clipboard
wl-paste | omapaint -        # same, via stdin
omapaint --annotate file.png # annotate mode (see contract below)
```

Opening a file for the user to edit is the primary agent use case. The app
starts in well under a second; the window class is `omapaint`.

## Annotate contract (screenshot pipeline)

`omapaint --annotate <file>` saves **in place** and exits when the user
presses **Ctrl+Enter** (Done). Exit code 0 means saved; non-zero means the
edit was discarded. The `omapaint-edit <file>` wrapper additionally copies
the result to the clipboard as image/png after a successful Done (the app
cannot do this itself — Wayland drops a client's clipboard on exit).

To hand the current screenshot flow to OmaPaint once:

```bash
omarchy screenshot --editor=omapaint-edit
```

Permanently (writes `~/.config/uwsm/env.d/20-omapaint`, next login):

```bash
omapaint-edit --install      # undo with: omapaint-edit --uninstall
```

## Scripting notes

- A waiting annotate session can be completed from a script with
  `wtype -M ctrl -k Return -m ctrl` once the window has focus
  (`hyprctl dispatch "hl.dsp.focus({ window = 'class:omapaint' })"`).
- Verify a clipboard result with `wl-paste --list-types` (expect
  `image/png`). The window must have been focused for clipboard reads.
- Do **not** try to draw via synthesized pointer events (none exist on
  Hyprland) and do not use the hidden `--demo` flag for real edits — it
  replays a fixed promo script.
- Editing is destructive-in-place in annotate mode; copy the file first if
  the original must be kept.
