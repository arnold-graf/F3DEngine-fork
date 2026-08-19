# Agent notes for F3DEngine

## Visual verification

Do **not** use desktop screenshots (`screencapture`, etc.) to verify rendering changes.

Use the headless render script instead:

```bash
./scripts/render_snapshot.sh build/snapshot.png
```

This will:

1. Build `f3dengine` if needed
2. Run `./f3dengine --snapshot <temp.bmp>` (CPU renderer only — no window)
3. Convert the BMP to PNG via `scripts/bmp_to_png.py` (stdlib only)
4. Fail if the image is effectively all black (average RGB below `MIN_BRIGHTNESS`, default `0.02`)

After graphics or presentation changes, run the script and confirm it passes. Open `build/snapshot.png` if you need to inspect the image manually.

## Common pitfalls

- **Presentation path:** Interactive mode presents the CPU `FrameBuffer` with an OpenGL textured fullscreen quad. `glDrawPixels` is unreliable with the engine's Y-down orthographic projection on macOS.
- **Snapshot mode:** `--snapshot` writes the raw `accumulationBuffer` via BMP; it does not use OpenGL.
- **Working directory:** Run commands from the repo root so `./gameDef/` and `./texture/` paths resolve.

## Build

```bash
make
make run          # interactive window
./scripts/render_snapshot.sh   # headless PNG verification
```
