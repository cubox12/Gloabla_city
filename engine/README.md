# GLOBAL ENGINE 0.2 — large-world runtime

Native C++17 / Android NativeActivity / OpenGL ES 3.0 runtime for GLOBAL CITY.

## Large-world architecture
- Procedural 4096 x 4096 world grid (64 x 64 chunks, each 64 world units).
- Lazy chunk generation around the player; 5 x 5 active window (25 resident chunks maximum).
- Chunks outside the active window are evicted from CPU memory and their GPU buffers are released.
- Deterministic streets, dashed road markings, sidewalks, facade variation, floor bands, rooftop ventilation, windows, street lamps and low-poly street trees.
- Collision data is kept only for resident chunks.
- Separate high-detail and low-detail chunk meshes. Windows and small props are omitted in low LOD.
- GPU-side per-chunk VBO cache; chunks are uploaded only when first entering the active window.
- Frustum-sphere and squared-distance culling before issuing draw calls.
- Stylized low-poly player, five pedestrians and a small delivery car use one dynamic streaming VBO.
- Color-graded fog shading softens distant geometry; a cached bitmap HUD reports mission stage and distance.
- Original real-time synthesized audio feedback for footsteps, package pickup and delivery.
- Camera follows the player. Android controls: virtual joystick on the lower-left (touch and drag) to move; drag anywhere on the right half to orbit the camera. Pinch zoom is disabled; camera distance is fixed.

## First playable mission — mission 01
`Misión 01: Primer reparto` now has a complete search-pickup-delivery loop:
1. Start on the city avenue and follow the HUD distance to the blue delivery car at (96, -6).
2. Enter its 4.25-unit pickup radius. The parcel appears on the character and the objective moves to (160, -64).
3. Follow the cyan beacon to the delivery area. Entering its 4.25-unit radius completes the mission and turns the beacon green.
4. Original synthesized sound effects play on footsteps, pickup and delivery.

Pickup and delivery are proximity-triggered; this slice does not yet include dialogue, vehicle driving, traffic, skeletal animation, minimap or save/load.

## Build and tests
```bash
cmake -S engine -B build-engine -DCMAKE_BUILD_TYPE=Release
cmake --build build-engine --parallel
ctest --test-dir build-engine --output-on-failure
```

## Low-end stress profile
`global_engine_stress` simulates prolonged traversal, chunk creation/eviction and a synthetic crowd workload. It reports CPU frame-time percentiles on the machine running the test and checks the resident chunk cap. It is a repeatable CPU-side stress test, not a GPU/thermal benchmark and not a substitute for profiling on a physical low-end Android device.

## Current limits
The world grid is finite and generated on demand. The renderer is still a colored-geometry prototype rather than a textured/PBR pipeline. There is no real-time shadow map, normal-mapped material system, traffic AI, vehicle physics, mission scripting framework or animation system. Android requires OpenGL ES 3.0.
