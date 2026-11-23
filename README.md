# GLOBAL CITY 0.4

Prototipo 3D para Godot 4.x, con perfil Android y renderer Compatibility.

## Estado del prototipo

- Plataforma plana de 140 × 140 unidades.
- Cuadrícula de calles perpendiculares con líneas centrales discontinuas.
- Aceras y pasos peatonales en las intersecciones.
- 16 parcelas vacías delimitadas; todavía no hay edificios ni vehículos.
- Personaje low-poly inmóvil, sin controles ni animaciones.
- Cámara orbital con giro táctil, zoom por pinza y controles de ratón en escritorio.
- HUD con instrucciones y estado del prototipo.
- Geometría estática agrupada por material para reducir nodos y llamadas de dibujo.
- Texturas procedurales de asfalto y césped de 128 × 128.
- Perfil móvil inicial sin sombras dinámicas.

## Abrir

1. Instala Godot 4.x.
2. Desde la raíz del repositorio, ejecuta `python3 tools/generate_textures.py` para crear los PNG locales.
3. Abre `project.godot` en Godot y ejecuta la escena principal.

## Controles

- Android: arrastra un dedo para girar la cámara; separa o junta dos dedos para acercar o alejar.
- Escritorio: mantén el botón izquierdo y arrastra para girar; usa la rueda para zoom.
- El personaje no se mueve en esta fase.

## Validación

Ejecuta `python3 tools/generate_textures.py` y luego `python3 tools/validate_project.py`.

GitHub Actions también intenta importar el proyecto y ejecutar la escena en modo headless con Godot 4.4.1. Revisa la pestaña Actions para ver el resultado real de CI.

## APK Android

No se incluye APK ni artefactos compilados. La exportación requiere las Export Templates de Godot, Android SDK y JDK configurados. Todavía no se han medido FPS ni consumo en un dispositivo real; el batching es una optimización estructural, no una cifra de rendimiento medida.

## Estructura

- `Main.gd`: escenario procedural, parcelas, cámara orbital y personaje estático.
- `Main.tscn`, `project.godot`: entrada y configuración.
- `tools/`: generación de texturas y validación.
- `.github/workflows/`: automatización de validación.
- `asset-register.csv`, `ART_DIRECTION.md`: procedencia de recursos y dirección visual.
