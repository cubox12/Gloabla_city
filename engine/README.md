# GLOBAL CITY-BETA 0.1 — GLOBAL ENGINE native runtime

Native C++17 / Android NativeActivity / OpenGL ES 3.0 runtime for GLOBAL CITY.

## Large-world architecture
- Procedural 4096 x 4096 world grid (64 x 64 chunks, each 64 world units).
- Lazy chunk generation around the player; 5 x 5 active window (25 resident chunks maximum).
- Chunks outside the active window are evicted from CPU memory and their GPU buffers are released.
- Deterministic streets, dashed road markings, sidewalks, setback building masses, recessed window panels, selective balconies, restrained storefronts, rooftop equipment, cylindrical street lamps and rounded low-poly tree canopies. Tree envelopes are kept outside building footprints and carriageways.
- Visual pass: procedural zebra crossings at all avenue intersections, varied storefront signs and striped awnings, occasional rooftop water tanks, and a vertical sky/horizon fog gradient. Crosswalk paint is retained in the low-detail mesh.
- Low-detail chunk storage reserves capacity for the expanded street-marking set to avoid repeated vector growth during sector generation.
- Collision data is kept only for resident chunks.
- Separate high-detail and low-detail chunk meshes. Windows and small props are omitted in low LOD.
- GPU-side per-chunk VBO cache with lazy LOD upload; only the visible detail level is uploaded, and the other level is created when it becomes necessary.
- Frustum-sphere and squared-distance culling before issuing draw calls.
- The protagonist and five pedestrians use smooth ellipsoid anatomy with separately animated thighs, shins, arms, hands, shoes, head and hair. The protagonist and pedestrians now share close-up eyes, irises, pupils, eyebrows, nose, ears and mouth, with varied skin and hair tones across pedestrians. Rear hair volume and a shirt collar soften the head/neck silhouette. The transient character mesh is preallocated for the added facial geometry. Rounded parts are streamed through one dynamic VBO. The delivery car uses softened bodywork and cylindrical tyres alongside its windows, grille, bumpers, lamps and hubs. Curved geometry now uses denser tessellation (10 rings x 16 segments for ellipsoids) and softened boxes have four face subdivisions, reducing visibly faceted silhouettes. Character torsos are generated from a tapered profile with waist, chest and shoulder transitions; the protagonist and five pedestrians share detailed facial geometry, individualized skin/hair tones and more varied muted clothing colors. These remain procedural models rather than authored photorealistic assets or a skeletal rig. Low-cost contact-shadow decals are retained under moving characters and the parked car.
- Saturated facade colors, stronger warm color grading, height-aware light tint and distance fog improve depth/readability without texture memory or shadow-map passes. The GLES renderer now carries world-space vertex normals and evaluates a per-pixel diffuse/hemisphere/specular response, world-space procedural surface variation for asphalt, vegetation and plaster facades, and a 1024 x 1024 directional depth shadow map with 2 x 2 PCF filtering. The shadow target is optional: devices that cannot create a depth-only framebuffer fall back to unshadowed PBR-lite lighting. This remains a lightweight mobile material pass, not a full metallic-roughness texture workflow. A cached bitmap HUD reports mission stage and distance; a compact live top-down minimap shows the road grid, player and mission objective. HUD/map geometry is preallocated to avoid per-frame growth. Shader compilation has a simpler fallback, EGL startup is logged, and failed GPU initialization keeps a visible diagnostic surface.
- Original synthesized audio feedback includes a rising pickup chime, a four-note delivery cue and short low-frequency footsteps.
- Camera follows the player. Android controls: large fixed-base virtual joystick in the lower-left with circular clamping and a small radial deadzone; the right side remains a separate camera-drag region. Touch pointers are tracked independently so camera movement cannot steal the joystick finger. Pinch zoom is disabled; camera distance is fixed.

## First playable mission — mission 01
`Misión 01: Primer reparto` now has a complete search-pickup-delivery loop:
1. Start on the city avenue and follow the HUD distance to the blue delivery car at (96, -6).
2. Enter its 4.25-unit pickup radius. The parcel appears on the character and the objective moves to (160, -64).
3. Follow the cyan beacon to the delivery area. Entering its 4.25-unit radius completes the mission and turns the beacon green.
4. Original synthesized sound effects play on footsteps, pickup and delivery.

Pickup and delivery are proximity-triggered; this slice does not yet include dialogue, vehicle driving, traffic AI, a skeletal rig, or save/load. The minimap is available; character motion is still procedural rather than a reusable animation rig.

## Build and tests
```bash
cmake -S engine -B build-engine -DCMAKE_BUILD_TYPE=Release
cmake --build build-engine --parallel
ctest --test-dir build-engine --output-on-failure
```

## Low-end stress profile
`global_engine_stress` simulates prolonged traversal, chunk creation/eviction and a synthetic crowd workload. It reports CPU frame-time percentiles on the machine running the test and checks the resident chunk cap. It is a repeatable CPU-side stress test, not a GPU/thermal benchmark and not a substitute for profiling on a physical low-end Android device.

## Current limits
The world grid is finite and generated on demand. The renderer remains an original procedural geometry pipeline with lightweight PBR-lite materials and generated surface variation. It does not yet have texture atlases, authored normal/roughness maps, photorealistic authored character meshes or a skeletal animation rig. There is no real-time shadow map, normal-mapped material system, traffic AI, vehicle physics, skeletal rig or mission scripting framework. Android requires OpenGL ES 3.0. Emulator smoke tests use Google SwiftShader; the Adreno 610 still needs physical-device performance, thermal and touch-feel testing.
