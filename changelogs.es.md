# Registro de cambios

[English](changelogs.md) · [한국어](changelogs.ko.md) · [日本語](changelogs.ja.md) · [简体中文](changelogs.zh-CN.md) · [繁體中文](changelogs.zh-TW.md) · **Español** · [Français](changelogs.fr.md) · [Deutsch](changelogs.de.md) · [Português](changelogs.pt.md) · [Русский](changelogs.ru.md) · [Italiano](changelogs.it.md)

Cambios importantes de Cliderness (creado por korscripterdev).

## [1.1.0] — 2026-11-15

Completa lo que la 1.0.0 dejó pendiente en la hoja de ruta.

### Añadido
- Búsqueda de caminos A* para las criaturas (montículo binario y hash, sube 1 y baja 3 bloques, guarda una ruta de 12 pasos).
- Atributos que crecen jugando hasta 30: cada habilidad entrena un atributo; Constitución sube al recibir daño (+2 de vida máxima), Voluntad al soportar extremos, Suerte con botín y críticos.
- Pesca: caña de pescar, picada a los 3-8 segundos, probabilidad 35% + Pesca / 2 + Suerte / 4, reliquias en la Dimensión Antigua.
- Habilidad de sigilo: pasar agachado junto a criaturas despiertas la entrena y te hace más silencioso.
- Los barcos derivan con vientos de más del 25% salvo que estén anclados o toquen la orilla.
- El Marinero (costas) vende contratos de tripulante; la tripulación rema sin aguante (+10% cada uno), achica 2 l por segundo y pesa 75 kg. Literas: balsa 0, bote 1, velero 2, mercante 4.
- El cielo antiguo: la segunda luna Veyra (ciclo de 3 días), un mundo anillado, el velo de éter, estrellas magenta y cian.
- El audio de Linux/BSD transmite PCM mezclado con 60 ms de adelanto.
- Ayuda en los 11 idiomas (63 temas) y nombres de bloques, objetos, biomas, clima y efectos.

### Cambiado
- Las versiones de DOS compilan y enlazan en la CI (Open Watcom 16 bits y DOS/4GW); aún no se han ejecutado en hardware ni en emulador.
- Tablas de idioma en DOS: 36000 bytes y 960 entradas.

## [1.0.0] — 2026-11-15

Fase 10 de la hoja de ruta (sistemas avanzados). Las diez fases están implementadas; lo que falta verificar está en ROADMAP.md.

### Añadido
- Audio PCM en vivo: Windows waveOut (mezcla con lo que suena), Linux/BSD con PulseAudio, PipeWire o ALSA, macOS con afplay. Todo se sintetiza con el DSP procedural.
- Pasos, truenos y sonido ambiente (lluvia, viento, tormenta, cuevas, Dimensión Ancestral, pájaros, cuervos, barcos). `--play-sfx` comprueba el dispositivo.
- 119 comandos nuevos (240 en total): formas de construcción, información, acciones como las teclas, almacenamiento, puntos de ruta, punto de muerte, interfaz y sistema.
- Paquetes: reglas install de CMake y CPack (ZIP, tar.gz), `tools/package.py` (con ZIP para DOS).

## [0.6.0] — 2026-11-14

Fase 9 de la hoja de ruta (la Dimensión Ancestral).

### Añadido
- Portales: fabrica un núcleo de portal en el yunque, colócalo y pulsa E para viajar; se construye un portal de regreso.
- La Dimensión Ancestral: semilla propia, 8 biomas (con grietas del vacío), bloques y plantas ancestrales, menas de éter y metal estelar.
- 7 criaturas ancestrales, ciudades y santuarios ancestrales, el altar ancestral con 9 recetas, reliquias.
- Clima de la dimensión (lluvia de estrellas, niebla silenciosa, viento de éter), reaparición por dimensión. 5 comandos nuevos (121 en total).

## [0.5.0] — 2026-11-14

Fase 8 de la hoja de ruta (barcos).

### Añadido
- Balsa, bote de remos, velero y barco mercante (5x3 casillas) con recetas.
- R para botar, E para subir/bajar, F para recoger un barco vacío. Gobierno con las teclas de movimiento (potencia -1..3, giro), ancla.
- Los remos gastan aguante; las velas usan la dirección y fuerza del viento. Habilidad de navegación.
- Flotabilidad y capacidad, bodegas, sobrecarga. 3 etapas de daño del casco, vías de agua, achique, reparación, olas, encallar, hundimiento.
- Sonidos de crujido y rotura. 12 comandos nuevos (116 en total).

## [0.4.0] — 2026-11-14

Fase 7 de la hoja de ruta (criaturas y combate).

### Añadido
- 22 criaturas definidas por datos: 11 animales, 8 monstruos y 3 PNJ.
- IA por objetivos (sobrevivir, atacar, refugiarse, volver a casa, descansar, deambular) con vista, oído, clima, oscuridad y sigilo.
- Aparición natural por bioma, hora, cuevas y agua; desaparecen lejos del jugador.
- Combate: F ataca, espadas, lanza, arco y flechas, críticos, armadura, sangrado y veneno. Los no muertos arden de día.
- 4 niveles de dificultad; se guardan criaturas, bajas y dificultad. Botín, carne cocinada, armadura de cuero, harina de hueso, pedernal, 15 recetas.
- Comercio con PNJ (E). 11 comandos nuevos (104 en total).

## [0.3.0] — 2026-11-14

Fase 6 de la hoja de ruta (simulación del mundo).

### Añadido
- ClideFluid: agua y lava que fluyen con niveles 1-15, conservación del volumen, mar y lagos que se rellenan, obsidiana, charcos de lluvia.
- Cubos, ríos en las tierras bajas y lagos subterráneos.
- Estructuras: minas abandonadas y ruinas de piedra con cajas de botín.
- 7 comandos nuevos (93 en total).

### Cambiado
- Ríos, lagos y estructuras cambian el terreno no explorado de los mundos antiguos.

## [0.2.0] — 2026-11-14

Fase 4 de la hoja de ruta (fabricación y construcción) y lo que faltaba de la fase 3.

### Añadido
- Pantalla de fabricación (K), 58 recetas en datos, mesa de trabajo, horno, fogata, yunque y combustible.
- Fundición, cocina y agricultura (azadas, tierra de cultivo, trigo y patatas, brotes que crecen hasta ser árboles).
- Cofres, camas y sueño, ranuras de equipo (cabeza, cuerpo, piernas, pies, espalda) y reparación.
- Estados: sangrado, fractura, intoxicación, resfriado, mojado, bien alimentado, descansado y sus remedios.
- 15 comandos nuevos (86 en total). Licencia: GPL-3.0 o posterior.

### Cambiado
- Las descripciones de los comandos nuevos se muestran en inglés (la interfaz sigue traducida en 11 idiomas).

## [0.1.0] — 2026-11-14

Primera base jugable (fases 1–3 completas; 4–6 y 10 parciales).

### Añadido
- Motor ISO C90 con adaptadores para consola de Windows, terminales POSIX, MS-DOS/FreeDOS y un adaptador sin pantalla para pruebas.
- Framebuffer CP437 / VGA-16 con salida de celdas modificadas y caracteres anchos.
- Generación entera por semilla, 38 biomas, cuevas, minerales, océanos y almacenamiento seguro de chunks.
- Movimiento, minería, colocación, herramientas y durabilidad, habilidades, supervivencia, temperatura, muerte y reaparición.
- Inventario, barra rápida y objetos con flotabilidad.
- ClideTime, 18 climas, vista del cielo; ClideConsole con 71 comandos.
- Ayuda de 45 temas en inglés y coreano, interfaz en 11 idiomas, DSP procedural con exportación WAV.
- Compilación con CMake, Make, Open Watcom y DJGPP; GitHub Actions; 293.610 comprobaciones.

### Limitaciones conocidas
- Aún sin fabricación, criaturas, combate, agua que fluye, barcos ni Dimensión Antigua.
- Sin audio PCM en vivo en Windows/Linux/macOS (funciona en silencio).
- Las versiones DOS aún no se han compilado ni ejecutado (sin herramientas locales).
- La ayuda solo está en inglés y coreano; los demás idiomas usan la ayuda en inglés.
