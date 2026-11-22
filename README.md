# GLOBAL CITY 0.3

Prototipo 3D para Godot 4.x, base Android con renderer Compatibility.

## Estado del prototipo

- Plataforma plana de 140 × 140 unidades.
- Cuadrícula de calles perpendiculares con marcas centrales discontinuas.
- Aceras básicas.
- Personaje low-poly inmóvil, sin controles ni animaciones.
- Cámara fija, cielo procedural e iluminación direccional.
- Geometría estática agrupada por material para reducir nodos y llamadas de dibujo.
- Texturas de asfalto y césped procedurales de 128 × 128.
- Perfil móvil inicial sin sombras dinámicas.

## Abrir

1. Instala Godot 4.x.
2. Desde la raíz del repositorio, ejecuta `python3 tools/generate_textures.py` para crear los PNG locales.
3. Abre `project.godot` en Godot y ejecuta la escena principal.

## Validación

`python3 tools/validate_project.py`

GitHub Actions ejecuta la validación de fuente y una comprobación headless con Godot 4.4.1. Los resultados de CI deben revisarse en la pestaña Actions.

## APK Android

No se incluye APK ni artefactos compilados. La exportación requiere las Export Templates de Godot, Android SDK y JDK configurados. Todavía no se han medido FPS ni consumo en un dispositivo real; el batching es una optimización estructural, no una cifra de rendimiento medida.

## Estructura

- `Main.gd`: escena procedural y personaje estático.
- `Main.tscn`, `project.godot`: entrada y configuración.
- `tools/`: generación de texturas y validación.
- `.github/workflows/`: automatización de validación.
- `asset-register.csv`, `ART_DIRECTION.md`: procedencia de recursos y dirección visual.
