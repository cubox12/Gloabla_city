# Misión 01 — Primer reparto

**Estado:** primera misión jugable del prototipo nativo  
**Objetivo:** localizar el coche azul, recoger el paquete y llevarlo al punto de entrega.

## Inicio
El jugador aparece en una avenida. El HUD muestra la distancia al coche azul y las instrucciones de la misión.

## Recorrido
1. **Encuentra el vehículo:** está en (96, -6), con una baliza cian.
2. **Recoge el paquete:** entra en un radio de 4.25 unidades del coche. El juego activa el paquete visible en la espalda del personaje y cambia el objetivo.
3. **Entrega el paquete:** sigue la baliza cian hasta (160, -64). Al entrar en el radio de 4.25 unidades, la baliza se vuelve verde y la misión queda completada.

La recogida y la entrega se activan automáticamente al llegar a sus zonas; no hay botón contextual en esta versión.

## Controles táctiles
- Joystick virtual abajo a la izquierda: mantener y arrastrar para caminar.
- Arrastrar la mitad derecha: girar la cámara.
- Sin zoom; distancia de cámara fija.
- El juego se ejecuta en horizontal.

## Contenido de esta versión
- Ciudad procedural con carreteras, aceras, líneas viales, edificios low-poly, ventanas, azoteas, farolas y árboles.
- Jugador con paquete visible durante la segunda fase y cinco peatones.
- Coche azul de reparto, baliza que cambia de ubicación y color según el objetivo.
- HUD con fases, distancia e instrucciones.
- Sonidos originales sintetizados en tiempo real: pasos, recogida y entrega.
- Streaming de sectores, LOD por distancia, frustum culling y buffers GPU persistentes.

## Límites conocidos
Esta es una primera misión completa como bucle de búsqueda-recogida-entrega, no un juego terminado: todavía no hay conducción del coche, tráfico, diálogo, animaciones esqueléticas, minimapa, guardado, menú de pausa ni pruebas físicas en dispositivos Android. Los efectos de sonido están sintetizados y son básicos.
