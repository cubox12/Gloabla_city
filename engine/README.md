# GLOBAL ENGINE 0.1 — núcleo

Motor de juego propio en C++17. Este núcleo no depende de Godot, Unity, SDL ni de un runtime de terceros.

## Compilar y probar

```bash
cmake -S engine -B build-engine -DCMAKE_BUILD_TYPE=Release
cmake --build build-engine --parallel
ctest --test-dir build-engine --output-on-failure
```

## Contenido actual
- Mundo procedural basado en datos C++.
- 16 edificios con fachadas y ventanas.
- Geometría de terreno, calles, aceras y mobiliario urbano.
- Movimiento del jugador con aceleración.
- Colisiones AABB para que el jugador no atraviese edificios.
- Datos de renderizado agrupados en una lista de cajas.
- Cinco peatones con movimiento ambiental sencillo.

## Próximos sistemas
1. Plataforma Android NativeActivity y renderizador OpenGL ES 3.
2. Buffer estático para geometría de ciudad y buffer dinámico para entidades.
3. Cámara móvil y controles multitáctiles.
4. Cargador de recursos glTF, texturas y materiales.
5. Escenas serializadas, audio, animaciones y herramientas de depuración.

El núcleo sí tiene pruebas C++ automatizadas. Todavía no es un editor ni un motor general completo; es la base del runtime específico para GLOBAL CITY.
