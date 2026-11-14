# Cliderness

[English](README.md) · [한국어](README.ko.md) · [日本語](README.ja.md) · [简体中文](README.zh-CN.md) · [繁體中文](README.zh-TW.md) · **Español** · [Français](README.fr.md) · [Deutsch](README.de.md) · [Português](README.pt.md) · [Русский](README.ru.md) · [Italiano](README.it.md)

*Sandbox de supervivencia en mundo abierto* — creado por **korscripterdev**

Cliderness es un sandbox de supervivencia libre dibujado solo con caracteres CP437 en 16 colores VGA sobre una cuadrícula de 80×25. Un único motor ISO C90 funciona en MS-DOS, FreeDOS, Windows, Linux, macOS y otros Unix. Sin historia ni final: explora, pica, construye, transforma el terreno y sobrevive.

## Qué funciona en 0.1.0

* Mundo 3D determinista por semilla (idéntico en 16 y 64 bits), 38 biomas, cuevas, minerales, océanos y arrecifes; solo se guardan los chunks modificados.
* Andar, correr, agacharse, nadar, trepar y caer; picar, colocar, cavar hacia abajo y construir hacia arriba; herramientas con durabilidad.
* Salud, aguante, hambre, sed, fatiga y temperatura corporal; inventario de 36 casillas con barra rápida y objetos que flotan o se hunden.
* Tiempo de juego con estaciones y fases lunares, 18 climas y vista del cielo (sol, luna, constelaciones, nubes, aves, meteoros, auroras, eclipses).
* **71** comandos que funcionan de verdad (permisos, autocompletado con Tab, historial, deshacer/rehacer).
* Ayuda sin conexión con 45 temas y 5 niveles, interfaz en 11 idiomas; F1 / Esc detienen el juego por completo; guardado seguro (archivo temporal + CRC).

Aún no: fabricación, criaturas y combate, agua que fluye, barcos, la Dimensión Antigua y audio PCM en vivo. Estado exacto en [ROADMAP.md](ROADMAP.md).

## Compilar

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
make && make test
sh builds/dos/build_watcom.sh 16
```

Ejecuta `build/cliderness`. `--help` muestra las opciones y `--version` la versión y el autor.

## Controles (por defecto)

`WASD` mover · `Shift` correr · `Ctrl`/`X` agacharse · `F` picar · `R` colocar · `>`/`<` bajar/subir · `E` recoger · `U` usar · `Tab` inventario · `C` personaje · `Shift dcho.`/`V` cielo · `F1`/`?` ayuda · `Esc` pausa · `/` consola. Todas las teclas se pueden cambiar.

## Documentación

[DESIGN.md](DESIGN.md) · [ARCHITECTURE.md](ARCHITECTURE.md) · [ROADMAP.md](ROADMAP.md) · [cambios](changelogs.es.md) · [commands](docs/commands/commands.md) · [DOS](docs/platforms/dos.md)

© 2026 korscripterdev · GPL-3.0-or-later · Ver [LICENSE](LICENSE).
