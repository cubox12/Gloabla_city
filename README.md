# GLOBAL CITY 0.5

Prototipo 3D para Godot 4.x, perfil Android y renderer Compatibility.

## Estado
- Plataforma 140 × 140, calles, aceras y pasos peatonales.
- 16 parcelas vacías delimitadas.
- Personaje controlable con CharacterBody3D y colisión.
- Cinco peatones NPC con movimiento ambiental sencillo.
- Cámara orbital que sigue al jugador, giro táctil y zoom por pinza.
- Controles táctiles en pantalla y teclado WASD/flechas.
- Texturas procedurales de asfalto, césped, concreto y pavimento (128 × 128).
- Geometría estática agrupada y sombras dinámicas desactivadas.

## Controles
- Android: usa el pad direccional en pantalla para caminar; arrastra fuera de los botones para girar la cámara; pinza para zoom.
- PC: WASD o flechas para moverte; arrastra con botón izquierdo para girar; rueda para zoom.

## Abrir y validar
1. Ejecuta `python3 tools/generate_textures.py`.
2. Abre `project.godot` con Godot 4.x.
3. Ejecuta la escena principal.
4. Valida con `python3 tools/validate_project.py`.

GitHub Actions importa y ejecuta la escena en modo headless con Godot 4.4.1.

## APK
No se incluye APK todavía. Requiere Export Templates, Android SDK y JDK configurados. El rendimiento no se ha medido en un dispositivo Android real.

## Estructura
- `Main.gd`: mundo, cámara, HUD, creación del jugador y peatones.
- `Player.gd`: controlador de movimiento.
- `Pedestrian.gd`: movimiento ambiental NPC.
- `tools/`: generación de texturas y validación.
- `assets/`: texturas PNG generadas localmente.
