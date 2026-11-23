# GLOBAL CITY — transición a GLOBAL ENGINE

GLOBAL CITY 0.6 se conserva como prototipo de referencia. La nueva línea de desarrollo es **GLOBAL ENGINE 0.1**, un motor propio en C++17 que se desarrolla por separado y no carga el runtime de Godot.

El núcleo vive en engine/ y se puede compilar y probar en PC sin Android SDK ni bibliotecas de terceros.

## Validar el núcleo nativo

```bash
cmake -S engine -B build-engine -DCMAKE_BUILD_TYPE=Release
cmake --build build-engine --parallel
ctest --test-dir build-engine --output-on-failure
```

La migración es gradual: el proyecto anterior permanece intacto como referencia hasta que el motor nuevo reproduzca las funciones del juego. No se versionan APKs, bibliotecas compiladas ni cachés.
