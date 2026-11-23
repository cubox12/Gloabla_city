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
- Geometría estática separada de la geometría dinámica.
- La ciudad estática se convierte a vértices y se sube una vez a la GPU por revisión del mundo.
- Un búfer de streaming separado se reserva para el jugador y los peatones.
- Cinco peatones con movimiento ambiental sencillo.

## Próximos sistemas
1. Renderizado por sectores de ciudad con frustum culling.
2. LOD de edificios, mobiliario y vehículos según distancia.
3. Sistema de escenas/entidades y recursos.
4. Cargador de recursos glTF/GLB, texturas y materiales.
5. Streaming de sectores para ampliar el mundo sin cargar toda la ciudad a la vez.
6. Audio, animaciones y herramientas de depuración.

El núcleo sí tiene pruebas C++ automatizadas. Todavía no es un editor ni un motor general completo; es la base del runtime específico para GLOBAL CITY.
