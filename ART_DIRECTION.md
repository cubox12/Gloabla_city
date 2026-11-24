# Dirección visual — GLOBAL CITY

## Objetivo visual
Ciudad urbana semi-realista, legible y creíble en móvil. La geometría de bajo coste debe conservar siluetas y proporciones naturales; no se aceptará que cajas de colores sustituyan indefinidamente a personajes, vehículos o arquitectura.

## Prioridad de producción
1. Sustituir el humanoide provisional de cajas por un personaje 3D riggeado, con anatomía, ropa, materiales y animaciones coherentes.
2. Sustituir los edificios prismáticos por módulos arquitectónicos con fachadas, ventanas insertadas, balcones, entradas, variaciones de planta y tejados diferenciados.
3. Incorporar materiales PBR de coste controlado, iluminación ambiental y sombras selectivas.
4. Mejorar vehículos, vegetación, mobiliario urbano y señalización con modelos de silueta reconocible.

## Reglas de urbanismo procedural
- La calzada tiene 8 unidades de ancho y las aceras se mantienen fuera de ella.
- Farolas, árboles, señales y mobiliario se colocan en franjas peatonales o alcorques; nunca en el centro de carriles.
- Ningún objeto de mobiliario o vegetación puede solaparse con edificios, fachadas, entradas o la zona libre de circulación.
- Las copas de los árboles deben respetar fachadas, balcones, postes y cables.
- Los accesos a edificios y las esquinas deben permanecer despejados.
- Las reglas de colocación se validan con pruebas deterministas por sector, además de inspección visual.

## Perfil móvil
- Objetivo inicial: Adreno 610, GLES 3.0.
- Usar LOD por distancia, instancing donde sea apropiado, atlas de materiales y geometría compartida.
- Sombras selectivas y resolución adaptativa; evitar transparencias y luces dinámicas por objeto.
- La calidad visual no se considera validada por el mero hecho de compilar: se requiere captura en dispositivo y revisión de clipping, iluminación, escala y rendimiento.

## Estado honesto
La implementación actual todavía usa cajas para el protagonista, peatones, coche y buena parte de la arquitectura. Es una base funcional de geometría procedural, no un resultado realista. Las siguientes mejoras visuales deben introducir activos 3D reales y materiales; añadir más cubos no cuenta como progreso de calidad.
