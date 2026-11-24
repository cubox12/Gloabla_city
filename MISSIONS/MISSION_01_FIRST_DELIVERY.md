# Mission 01 — Primer reparto

**Estado:** prototipo incompleto / primera entrega jugable  
**Objetivo actual:** localizar el coche azul de reparto y llegar a la baliza cian.

## Inicio
El jugador aparece en una avenida de la ciudad. El HUD muestra la misión y la distancia aproximada al vehículo.

## Recorrido
- Salida: (0, 0) en el plano X/Z.
- Vehículo: (96, -6).
- Baliza: junto al coche, en (96, -2).
- Radio de activación: 4.25 unidades alrededor del vehículo.
- Al entrar en el radio, la misión pasa de `InProgress` a `Completed` y la baliza cambia de cian a verde.

## Controles táctiles
- Mantener y deslizar el lado izquierdo: movimiento.
- Deslizar el lado derecho: rotar la cámara.
- Pinza con dos dedos: acercar o alejar la cámara.

## Contenido ya presente
- Ciudad procedural por chunks, con carreteras, aceras, líneas discontinuas, edificios de fachadas variadas, bandas de planta, azoteas, ventanas y farolas.
- Jugador y cinco peatones low-poly.
- Vehículo compacto azul de reparto y baliza de destino.
- HUD con nombre de misión, objetivo, distancia e instrucciones.
- Streaming de sectores, LOD por distancia, culling y buffers GPU persistentes.

## Pendiente para convertirla en misión completa
- Interacción explícita para recoger el paquete.
- Diálogo/encargo inicial.
- Carga visual del paquete y destino de entrega.
- Tráfico, conducción y física del vehículo.
- Animaciones, audio, efectos, colisiones del coche y peatones.
- Marcador direccional y minimapa.
- Guardado y reanudación.

La misión actual termina al encontrar el vehículo. Es una prueba de recorrido y de la integración motor-juego, no una misión comercial terminada.
