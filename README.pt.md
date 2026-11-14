# Cliderness

[English](README.md) · [한국어](README.ko.md) · [日本語](README.ja.md) · [简体中文](README.zh-CN.md) · [繁體中文](README.zh-TW.md) · [Español](README.es.md) · [Français](README.fr.md) · [Deutsch](README.de.md) · **Português** · [Русский](README.ru.md) · [Italiano](README.it.md)

*Sandbox de sobrevivência em mundo aberto* — criado por **korscripterdev**

Cliderness é um sandbox de sobrevivência livre desenhado apenas com caracteres CP437 em 16 cores VGA numa grade de 80×25. Um único motor ISO C90 roda em MS-DOS, FreeDOS, Windows, Linux, macOS e outros Unix. Sem história e sem final: explore, minere, construa, transforme o terreno e sobreviva.

## O que funciona na 0.1.0

* Mundo 3D determinístico por semente (idêntico em 16 e 64 bits), 38 biomas, cavernas, minérios, oceanos e recifes; só os chunks alterados são salvos.
* Andar, correr, agachar, nadar, escalar e cair; minerar, colocar, cavar para baixo e construir para cima; ferramentas com durabilidade.
* Saúde, fôlego, fome, sede, cansaço e temperatura corporal; inventário de 36 espaços com barra rápida e itens que flutuam ou afundam.
* Tempo de jogo com estações e fases da lua, 18 climas e visão do céu (sol, lua, constelações, nuvens, pássaros, meteoros, auroras, eclipses).
* **71** comandos que realmente funcionam (permissões, autocompletar com Tab, histórico, desfazer/refazer).
* Ajuda offline com 45 tópicos em 5 níveis, interface em 11 idiomas; F1 / Esc pausam o jogo por completo; salvamento seguro (arquivo temporário + CRC).

Ainda não: artesanato, criaturas e combate, água corrente, navios, a Dimensão Antiga e áudio PCM ao vivo. Status exato em [ROADMAP.md](ROADMAP.md).

## Compilar

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
make && make test
sh builds/dos/build_watcom.sh 16
```

Execute `build/cliderness`. `--help` mostra as opções e `--version` a versão e o autor.

## Controles (padrão)

`WASD` mover · `Shift` correr · `Ctrl`/`X` agachar · `F` minerar · `R` colocar · `>`/`<` descer/subir · `E` pegar · `U` usar · `Tab` inventário · `C` personagem · `Shift direito`/`V` céu · `F1`/`?` ajuda · `Esc` pausa · `/` console. Todas as teclas podem ser alteradas.

## Documentação

[DESIGN.md](DESIGN.md) · [ARCHITECTURE.md](ARCHITECTURE.md) · [ROADMAP.md](ROADMAP.md) · [alterações](changelogs.pt.md) · [commands](docs/commands/commands.md) · [DOS](docs/platforms/dos.md)

© 2026 korscripterdev · GPL-3.0-or-later · Veja [LICENSE](LICENSE).
