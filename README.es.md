# Cliderness

[English](README.md) · [한국어](README.ko.md) · [日本語](README.ja.md) · [简体中文](README.zh-CN.md) · [繁體中文](README.zh-TW.md) · **Español** · [Français](README.fr.md) · [Deutsch](README.de.md) · [Português](README.pt.md) · [Русский](README.ru.md) · [Italiano](README.it.md)

*Sandbox de supervivencia en mundo abierto* — creado por **korscripterdev**

Cliderness es un sandbox de supervivencia libre dibujado solo con caracteres CP437 en 16 colores VGA sobre una cuadrícula de 80×25. Un único motor ISO C90 funciona en MS-DOS, FreeDOS, Windows, Linux, macOS y otros Unix. Sin historia ni final: explora, pica, construye, transforma el terreno y sobrevive.

## Qué funciona en 1.0.0

* Mundo 3D determinista a partir de una semilla (idéntico en 16 y 64 bits), 46 biomas, cuevas, menas, océanos, ríos y lagos subterráneos; solo se guardan los chunks modificados.
* Caminar, correr, agacharse, nadar, minar, colocar, herramientas con durabilidad; salud, aguante, hambre, sed, fatiga, temperatura corporal, heridas y enfermedades.
* Fabricación (87 recetas), mesa de trabajo, horno, hoguera, yunque y altar, cultivos y árboles, cofres, camas, equipo y mochilas, reparaciones.
* ClideFluid: agua y lava que fluyen conservando el volumen, presas, charcos de lluvia, cubos; minas abandonadas, ruinas y ciudades ancestrales.
* 29 criaturas (animales, monstruos, PNJ) con IA por objetivos, vista y oído, rutinas de día y de noche; combate cuerpo a cuerpo y con arco, armaduras, botín, comercio, 4 dificultades.
* Barcos: balsa, bote de remos, velero y barco mercante; remos y velas con viento, bodegas, flotabilidad y sobrecarga, 3 etapas de daño del casco, inundación, hundimiento.
* La Dimensión Ancestral: portales, 8 biomas (con grietas del vacío), menas, plantas y criaturas ancestrales, el altar ancestral.
* Estaciones y fases lunares, 21 tipos de clima, vista del cielo, sonido procedural (PCM en vivo en Windows, Linux y macOS; altavoz de PC en DOS).
* **240** comandos, ayuda sin conexión con 62 temas en 5 niveles, interfaz en 11 idiomas, guardado seguro (archivo temporal + CRC).

Límites: la ayuda y los nombres de objetos solo existen en inglés y coreano (los demás idiomas muestran inglés). Los ejecutables de DOS se compilan en la CI pero aún no se han probado en hardware real ni en un emulador. Detalles en [ROADMAP.md](ROADMAP.md).

## Compilar

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
make && make test
sh builds/dos/build_watcom.sh 16
python tools/package.py            # ZIP / tar.gz (CPack)
```

Ejecuta `build/cliderness` (busca `data/` junto al ejecutable o en la carpeta actual). `--help` muestra las opciones, `--version` la versión y el autor, `--play-sfx rain` comprueba el dispositivo de audio.

## Controles (por defecto)

`WASD` mover · `Shift` correr · `Ctrl`/`X` agacharse · `F` atacar/minar · `R` colocar · `>`/`<` bajar/subir · `E` recoger/subir a bordo/comerciar · `U` usar · `K` fabricar · `Tab` inventario · `C` personaje · `Shift derecho`/`V` cielo · `F1`/`?` ayuda · `Esc` pausa · `/` consola. Todas las teclas se pueden cambiar.

## Documentación

[DESIGN.md](DESIGN.md) · [ARCHITECTURE.md](ARCHITECTURE.md) · [ROADMAP.md](ROADMAP.md) · [Cambios](changelogs.es.md) · [commands](docs/commands/commands.md) · [DOS](docs/platforms/dos.md)

© 2026 korscripterdev · GPL-3.0-or-later · Ver [LICENSE](LICENSE).
