# Dirección visual — GLOBAL CITY

## Pilares visuales
1. **Ciudad creíble:** fachadas sobrias, volúmenes escalonados, balcones, ventanas insertadas, aceras y mobiliario alineado con la calle.
2. **Personajes legibles:** silueta humana proporcionada, articulaciones visibles y movimiento natural; evitar personajes ensamblados con cajas.
3. **Móvil primero:** geometría low-poly suavizada, sombreado por vértice, LOD y presupuesto controlado para GLES 3.0 / Adreno 610.

## Reglas de mundo
- La calzada tiene 8 unidades de ancho; las aceras quedan fuera de ella.
- Árboles: tronco cilíndrico y copa elipsoidal; su envolvente completa debe quedar fuera de edificios y carriles.
- Farolas: poste cilíndrico en la franja peatonal; brazo orientado hacia la calzada sin cruzar el paso libre.
- La geometría de peatones no puede atravesar edificios.
- Los edificios se construyen con podio y torre con retranqueo, ventanas insertadas y balcones selectivos.
- Las reglas de colocación se validan por sector con pruebas deterministas.

## Materiales y luz
- Paleta de piedra, estuco, ladrillo y vidrio de saturación moderada.
- Variación de luz por forma en las mallas curvas y suavizado en aristas arquitectónicas.
- Siguiente fase: materiales texturizados, mapas normales, iluminación PBR y sombras selectivas; no declararlos terminados antes de probarlos en el dispositivo.

## Perfil móvil y verificación
- Objetivo inicial: Adreno 610, GLES 3.0.
- Usar LOD por distancia, VBO persistentes y geometría compacta.
- Medir FPS, GPU, temperatura y memoria en dispositivo físico.
- Una compilación o un emulador no certifican la calidad visual en el Adreno 610.

## Estado honesto
El protagonista ya no se dibuja con seis cajas: usa anatomía elipsoidal segmentada y movimiento procedural de brazos y piernas. Árboles y farolas usan primitivas redondeadas; los edificios tienen podio, torre retranqueada y balcones selectivos. La escena sigue siendo low-poly procedural, sin texturas PBR ni sombras en tiempo real; todavía no es realismo fotográfico.
