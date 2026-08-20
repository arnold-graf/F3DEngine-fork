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

## Code Style

This project should use simple yet modern C++. We always spell out names
completely, we don’t use abbreviations outside of very common conventions (i
instead of index is OK), and we don’t use overly academic jargon.

## Documentation

Whenever we make larger changes to the codebase, or gain important new
functionality, we update the README.md to reflect that.

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

## Performance overlay

The in-game overlay (bottom-right) shows:

- **FPS** — smoothed frames per second
- **frm** — frame time in milliseconds
- **CPU** — process CPU usage (can exceed 100% on multi-core; uses `getrusage`)
- **cor** — CPU % per hardware thread (better for comparing efficiency across machines)
- **cap** — FPS cap state (default 120)

Controls:

- **F** — cycle FPS cap: 60 → 120 → uncapped
- **0** — toggle overlay visibility (hidden by default)

CLI:

```bash
./f3dengine --fps-cap 120    # default
./f3dengine --fps-cap 60
./f3dengine --no-fps-cap     # uncapped
```

Game speed is frame-rate independent (movement, physics, and animated sectors scale with real elapsed time).

### Voxel terrain level

```bash
./f3dengine --level ./gameDef/savedLevelVoxel.json
./f3dengine --snapshot build/snapshot.png --level ./gameDef/savedLevelVoxel.json
```

Wattage is not available in-app without elevated privileges. On macOS use `sudo ./scripts/measure_power.sh 10` while the game runs. For external CPU sampling use `./scripts/measure_cpu.sh 10`.
