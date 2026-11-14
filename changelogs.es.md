# Registro de cambios

[English](changelogs.md) · [한국어](changelogs.ko.md) · [日本語](changelogs.ja.md) · [简体中文](changelogs.zh-CN.md) · [繁體中文](changelogs.zh-TW.md) · **Español** · [Français](changelogs.fr.md) · [Deutsch](changelogs.de.md) · [Português](changelogs.pt.md) · [Русский](changelogs.ru.md) · [Italiano](changelogs.it.md)

Cambios importantes de Cliderness (creado por korscripterdev).

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
