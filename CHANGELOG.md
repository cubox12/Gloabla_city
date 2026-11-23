# Changelog

## 0.5 — movilidad, peatones y materiales

- El personaje ahora usa CharacterBody3D, colisión cápsula, aceleración y movimiento direccional relativo a la cámara.
- Añadidos controles táctiles en pantalla y teclado WASD/flechas.
- La cámara sigue al personaje conservando giro y zoom.
- Añadidos cinco peatones low-poly con recorridos circulares ligeros.
- Añadida colisión física del suelo.
- Aceras con textura procedural de concreto; el generador crea también una textura de pavimento.
- El personaje y los NPC mantienen geometría sencilla y sin animaciones costosas.
- Sin mediciones FPS reales; pendiente probar en Android físico.

## 0.4 — city planning and mobile camera

- Added 16 empty, subtly tinted parcel pads with thin borders, merged into two static material batches. No buildings have been placed.
- Added zebra crossings at the 5 × 5 street intersections. Crosswalks share the existing road-marking batch.
- Replaced the fixed camera with an orbit pivot: one-finger drag rotates the view and pinch zoom changes camera distance on touch devices.
- Added desktop mouse drag and mouse-wheel zoom for editor/desktop testing.
- Added HUD instructions and parcel/player status.
- Kept the character completely static; no movement controller or physics body was introduced.
- Kept Compatibility rendering, small procedural textures and disabled dynamic shadows.
- Extended the static validator to check the new feature contract.
- No FPS claim is made: Godot is not installed in the authoring environment and no device profiling has been performed.
 
## 0.3 — validación continua y paquete limpio

- Añadida validación estática de referencias, renderer, batching, texturas y ausencia de binarios.
- Añadido workflow de GitHub Actions para importar y lanzar la escena con Godot 4.4.1 en modo headless.
- Añadidos .gitattributes para finales de línea consistentes.
- Se mantiene la escena base sin edificios, vehículos ni movimiento del personaje.

## 0.2 — static batching and mobile baseline

- Merged road, sidewalk, ground and road-marking boxes into one mesh batch per material.
- Merged the static character's body parts into four material batches.
- Reduced the source-level MeshInstance3D count from 169 in 0.1 to 8 in 0.2 (about 95.3% fewer instances). This is a structural count, not a measured FPS or frame-time result.
- Added small procedural, tileable 128×128 asphalt and grass textures.
- Disabled dynamic directional-light shadows in the low-cost baseline.
- Added art direction, asset provenance, ignore rules and a static validation script.
- Kept the player character immobile and did not add buildings, vehicles, traffic or gameplay controls.


## 0.6 — Primera ciudad construida
- Sustituye las parcelas vacías por 16 edificios low-poly con alturas y fachadas variadas.
- Añade ventanas, entradas y colisiones para impedir que el jugador atraviese los edificios.
- Añade árboles estilizados, farolas y bancas con geometría agrupada por material.
- Conserva controles móviles, peatones, calles y texturas de la versión anterior.
- Perfil gráfico Godot Compatibility para Android; falta medir rendimiento en dispositivo real.
