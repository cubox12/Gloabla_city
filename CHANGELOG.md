# Changelog

## 0.2 — static batching and mobile baseline

- Merged road, sidewalk, ground and road-marking boxes into one mesh batch per material.
- Merged the static character's body parts into four material batches.
- Reduced the source-level MeshInstance3D count from 169 in 0.1 to 8 in 0.2 (about 95.3% fewer instances). This is a structural count, not a measured FPS or frame-time result.
- Added small procedural, tileable 128×128 asphalt and grass textures.
- Disabled dynamic directional-light shadows in the low-cost baseline.
- Added art direction, asset provenance, ignore rules and a static validation script.
- Kept the player character immobile and did not add buildings, vehicles, traffic or gameplay controls.

## 0.3 — validación continua y paquete limpio

- Añadida validación estática de referencias, renderer, batching, texturas y ausencia de binarios.
- Añadido workflow de GitHub Actions para importar y lanzar la escena con Godot 4.4.1 en modo headless.
- Añadidos .gitattributes para finales de línea consistentes.
- Se mantiene la escena base sin edificios, vehículos ni movimiento del personaje.
