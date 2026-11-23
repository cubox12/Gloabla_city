# GLOBAL ENGINE 0.2 — large-world runtime

Native C++17 / Android NativeActivity / OpenGL ES 3.0 runtime for GLOBAL CITY.

## Large-world architecture
- Procedural 4096 x 4096 world grid (64 x 64 chunks, each 64 world units).
- Lazy chunk generation around the player; 5 x 5 active window (25 resident chunks maximum).
- Chunks outside the active window are evicted from CPU memory and their GPU buffers are released.
- Deterministic procedural buildings, facade variation, windows, sidewalks, roads and street props.
- Collision data is kept only for resident chunks.
- Separate high-detail and low-detail chunk meshes. Windows and small props are omitted in low LOD.
- GPU-side per-chunk VBO cache; chunks are uploaded only when first entering the active window.
- Frustum-sphere and distance culling before issuing draw calls.
- Dynamic player/NPC geometry uses a separate streaming buffer.
- Camera follows the player.

## Build and tests
```bash
cmake -S engine -B build-engine -DCMAKE_BUILD_TYPE=Release
cmake --build build-engine --parallel
ctest --test-dir build-engine --output-on-failure
```

## Low-end stress profile
`global_engine_stress` simulates prolonged traversal, chunk creation/eviction and a synthetic crowd workload. It reports CPU frame-time percentiles on the machine running the test and checks the resident chunk cap. It is a repeatable CPU-side stress test, not a GPU/thermal benchmark and not a substitute for profiling on a physical low-end Android device.

## Current limits
The world grid is finite and generated on demand. This is a streaming/procedural foundation, not yet a full traffic, vehicle, mission, animation, texture-streaming or save-game system. Android uses OpenGL ES 3.0 and requires a device that supports it.
