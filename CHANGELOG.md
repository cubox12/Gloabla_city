# Changelog

## GLOBAL ENGINE 0.1 — núcleo experimental

- Introduce un motor C++17 independiente, separado de la implementación anterior en Godot.
- Añade un mundo procedural de 16 edificios, geometría de ciudad, jugador y colisión AABB.
- Añade pruebas automatizadas del núcleo con CMake y CTest.
- La migración conserva temporalmente los archivos de GLOBAL CITY 0.6 como referencia.
- El núcleo se valida en Linux; la compilación Android y las pruebas en el dispositivo quedan como hitos separados.

## 0.6 — Primera ciudad construida
- Sustituye las parcelas vacías por 16 edificios low-poly con alturas y fachadas variadas.
- Añade ventanas, entradas y colisiones para impedir que el jugador atraviese los edificios.
- Añade árboles estilizados, farolas y bancas con geometría agrupada por material.
- Conserva controles móviles, peatones, calles y texturas de la versión anterior.
- Perfil gráfico Godot Compatibility para Android; falta medir rendimiento en dispositivo real.

## 0.5 — movilidad, peatones y materiales
- El personaje usa CharacterBody3D, colisión cápsula, aceleración y movimiento relativo a la cámara.
- Añadidos controles táctiles y teclado WASD/flechas.
- La cámara sigue al personaje conservando giro y zoom.
- Añadidos cinco peatones low-poly con recorridos circulares ligeros.
- Añadida colisión física del suelo y texturas procedurales.
- Sin mediciones FPS reales; pendiente probar en Android físico.
