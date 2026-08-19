# Performance optimizations (planned / future)

This document tracks performance improvements beyond the two implemented in the renderer refactor (`rayView` workspace reuse and CPU framebuffer presentation).

## Completed

### 1. Per-ray `rayView()` workspace reuse
- **Was:** Every horizontal ray deep-copied `lvl.sectorPack`, mutated wall lists, sorted, and allocated new `vector`s.
- **Now:** Each thread owns a `RayViewWorkspace` with pre-reserved `sectorHits` and `view` buffers. Rays read geometry from `lvl.sectorPack` in place and only store intersection results.

### 2. CPU framebuffer instead of OpenGL immediate-mode quads
- **Was:** Every surface built a `vector<QuadData>`, merged under a mutex, then submitted with `glBegin(GL_QUADS)` / `glVertex2f` per quad.
- **Now:** Pixels are written directly into `vecSys::FrameBuffer` via `fillOpaqueRect()`, then presented once per frame with `glDrawPixels()`.

---

## High priority (next)

### 3. Persistent thread pool / OpenMP parallel loops
- **Was:** `drawBaseWorld()` and `rayCast()` spawned and joined `std::thread::hardware_concurrency()` threads every frame.
- **Now:** OpenMP `#pragma omp parallel for` on sky/floor rows and ray columns (workers persist for the process lifetime).

### 4. Wall vertical spans (Doom-style columns)
- **Was:** `basicColumnFill()` emitted one rectangle per texture row (~128 quads per wall column), each calling `applyLighting()` per row.
- **Now:** One screen-pixel row per step; lighting sampled at column top/bottom and lerped vertically (2 light evaluations per wall column instead of ~128).

### 5. Eliminate mutex / merge passes in threaded draws
- **Problem:** Partially solved by writing columns directly to the framebuffer without merging quad lists.
- **Follow-up:** If tile-based rendering is added later, give each thread a sub-buffer to avoid false sharing on adjacent columns.

---

## Medium priority

### 6. Dynamic ray count / resolution scaling
- Tie `numRays` to performance budget (e.g. cap at 160 columns on slow hardware, scale with window width on fast hardware).
- Optional: render at lower internal resolution and upscale the framebuffer.

### 7. Spatial sector culling
- With only six sectors this is minor today; becomes important for larger levels.
- Grid or BSP over sector AABBs; only test sectors whose bounds intersect the ray frustum.

### 8. Texture pointer caching
- Hot paths still do `textures["./textures/....ppm"]` map lookups.
- Resolve texture pointers once per frame (or store indices on `Sector::Wall`).

### 9. Fixed-point math in hot paths
- Replace double ray/angle math in inner loops with 16.16 fixed-point where precision allows.
- Relevant if targeting very slow CPUs or a DOS-style port.

---

## Lower priority / lighting-specific

### 10. Per-column light sampling for walls
- Wall columns share the same world XY; only elevation varies vertically. Cache `computeLighting()` at column base and interpolate, or sample once per column.

### 11. Skip `illuminated()` for omnidirectional lights
- **Done:** Default point lights with `halfFOVH/V >= pi` skip the angle test in `computeLighting()`.

### 12. Shadow rays
- Not implemented. Would require ray tests from each shaded point toward each light — expensive; only consider for small light counts and after span-based rendering.

---

## Architectural (longer term)

### 13. Pure SDL surface blit (no OpenGL)
- Drop GL entirely: present `FrameBuffer::displayPixels` with `SDL_UpdateTexture` / `SDL_RenderCopy`.
- Simplifies portability and removes the last GL dependency for a software renderer.

### 14. Level editor on same framebuffer path
- `levelEditor.h` still uses immediate-mode GL lines; unify on `FrameBuffer` drawing primitives.

### 15. Binary level format
- JSON parse at load is fine for dev; a compact binary format would speed startup for large levels.

---

## Profiling suggestions

Before further work, measure frame time split:
1. `rayView` + intersection
2. `basicColumnFill` / floor fills
3. `fillOpaqueRect` pixel writes
4. `presentFramebuffer` / `glDrawPixels`

On macOS: Instruments (Time Profiler). Simple in-engine timing around `drawBaseWorld`, `rayCast`, and `presentFramebuffer` is enough to validate which item above matters next.
