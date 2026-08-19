# F3DEngine

Forked from <https://github.com/alexcdesilets/F3DEngine>.

An experimental 2.5D raycasting engine written in C++23. Levels are built from
polygon **sectors** (rooms and platforms with independent floor heights),
rendered with a raycaster extended to support vertical geometry, textured floors
and ceilings, skyboxes, and billboards.

## Features

### Rendering

- **Wall raycasting** — horizontal rays cast from the player; wall columns are drawn with per-wall textures and distance-based darkening.
- **Floors and ceilings** — perspective-correct floor and underside fills inside sectors, with per-sector textures and rotation.
- **Skybox** — scrolling night-sky texture above the horizon, tied to player yaw.
- **Billboards** — vertical sprite-like surfaces (structure in place; sample level uses sectors only).
- **Multi-threaded draw prep** — sky and floor quad generation runs across CPU threads (`std::thread`; OpenMP is included for other uses).

### World model

- **Sectors** — closed 2D polygons with:
  - `floatingHeight` and `baseHeight` (platform thickness)
  - Per-wall height, barrier flags, and textures
  - Floor, bottom (underside), and base-wall textures
  - Move, rotate, and orbit transforms
- **Player** — FPS movement with wall sliding, sector-aware collision, mouse look (pitch + yaw), jumping, and gravity.
- **Portals** — `portal.h` defines portal pairs with proportional teleport mapping (not wired into gameplay yet).
- **Lights** — `light.h` defines point lights with angular cutoffs and flicker.

### Data and tools

- Levels and texture lists are stored as **JSON** (`gameDef/savedLevel.json`, `gameDef/textures.json`).
- Textures are **PPM** images in `textures/`.
- A **level editor** skeleton exists in `levelEditor.h` (2D outline overlay); full editor mode is not hooked up to input yet.
- `keep_safe/` holds older experiments (collision, raycast, sorting) kept for reference.

### vecSys

A header-only 2D/3D math library in `vecSys/`:

| Module | Purpose |
|--------|---------|
| `vec2`, `vec3`, `vec4`, `vecRGB`, `vecRGBA` | Vectors and colors |
| `line2`, `line3`, `pline3`, `pLine2` | Lines and polylines |
| `shape2` | Closed polygons (point-in-poly, intersection, transforms) |
| `mat4`, `quaternion` | 3D transforms |
| `mesh3`, `triangleStrip3`, `quadPLane3`, `bezierSurface3` | 3D geometry (for future or tooling use) |
| `frameBuffer` | Software framebuffer utilities |

## Controls

| Input | Action |
|-------|--------|
| **W / A / S / D** | Move (strafe on A/D when not moving forward/back) |
| **Mouse** | Look (yaw and pitch) |
| **Space** | Jump |
| **Shift** | Sprint |
| **I** | Toggle menu (releases mouse capture) |
| **F** | Toggle 120 FPS cap |
| **0** | Toggle performance overlay (hidden by default) |
| **Escape** | Release mouse capture (click window to recapture) |

Run from the repository root so `./gameDef/` and `./textures/` paths resolve correctly.

### Performance overlay

Bottom-right HUD: FPS, frame time (ms), process CPU %, CPU % per core, and FPS cap status.

```bash
./f3dengine --fps-cap 120   # default
./f3dengine --no-fps-cap      # uncapped benchmark
```

Package power (macOS, requires sudo): `sudo ./scripts/measure_power.sh 10`

## Requirements

- **C++23** compiler (GCC 13+, Clang 16+, or MSVC with `/std:c++latest`)
- [SDL2](https://github.com/libsdl-org/SDL) (video + OpenGL context)
- OpenGL 2.1 (legacy immediate-mode `glBegin` / `glVertex` pipeline)
- [nlohmann/json](https://github.com/nlohmann/json) (header-only; not vendored in this repo)
- **OpenMP** — `globals.h` includes `<omp.h>`; use a compiler with OpenMP support, or remove that include if you build without it

## Building

### Windows

A batch script is provided for MinGW-style setups:

```bat
compile.bat
```

This produces `main.exe` linked against SDL2, OpenGL32, and OpenMP.

### Linux / macOS

Install SDL2 (and on macOS, `libomp`), then build from the repo root:

```bash
# macOS
brew install sdl2 libomp
make
make run

# Linux (Debian/Ubuntu)
sudo apt install libsdl2-dev libomp-dev curl
make
make run
```

The Makefile downloads [nlohmann/json](https://github.com/nlohmann/json) into `third_party/` on first build if it is not already present. Run `make clean` to remove the binary.

## Project layout

```
F3DEngine/
├── main.cpp              # SDL window, event loop, startup
├── globals.h             # Shared state, includes, game constants
├── rayCast.h             # Sky, floor, wall, and billboard rendering
├── Player.h              # Movement, collision, jump physics
├── sector.h              # Sector geometry and walls
├── level.h               # Level container and raycast packaging
├── billboard.h           # Billboard sprites
├── portal.h              # Portal math (future use)
├── light.h               # Light model (future use)
├── texture.h             # PPM texture loading and sampling
├── glCallBacks.h         # Display, resize, mouse
├── keyHandler.h          # Keyboard movement
├── levelEditor.h         # Editor overlay (partial)
├── IOUtils/              # JSON save/load for levels and textures
├── vecSys/               # Math library
├── gameDef/              # Level and texture manifest JSON
├── textures/             # PPM texture assets
├── keep_safe/            # Archived experiments
├── Makefile              # macOS / Linux build
└── compile.bat           # Windows build script
```

## Level format

Levels are JSON objects with `sectorList` and `billboardList`. Each sector has an `outline` (polygon points), height fields, texture paths, and a `walls` array aligned with outline edges. See `gameDef/savedLevel.json` for a full example.

To register textures, list PPM paths in `gameDef/textures.json`. Textures are loaded at startup before the level.

## Architecture (high level)

```
main loop
  ├── poll SDL events (keys, mouse, resize)
  ├── gamePLayKeys()          → player input
  ├── pl.physicUpdate()       → gravity, jump, collision
  ├── lvl.update()            → package sectors/billboards for raycast
  └── RayCast::drawBaseWorld() → sky + default floor
      RayCast::rayCast()       → sector walls, sector floors, billboards
```

Rendering uses an orthographic 2D OpenGL projection; the “3D” effect is entirely software raycasting into screen-space quads.

## Status and limitations

- Personal project — APIs and naming are inconsistent in places; some features are stubs.
- OpenGL 2.1 immediate mode only; no modern GPU pipeline.
- Sample level animates one sector every frame (`glCallBacks.h`) as a demo.
- Title screen, pause, options, and full level editor states exist in `GAMESTATES` but are mostly unimplemented.
- No CMake; use `Makefile` (macOS/Linux) or `compile.bat` (Windows).

## License

GNU General Public License v3 — see [LICENSE](LICENSE).
