# Cliderness

[English](README.md) · [한국어](README.ko.md) · [日本語](README.ja.md) · [简体中文](README.zh-CN.md) · [繁體中文](README.zh-TW.md) · [Español](README.es.md) · [Français](README.fr.md) · [Deutsch](README.de.md) · **Português** · [Русский](README.ru.md) · [Italiano](README.it.md)

*Sandbox de sobrevivência em mundo aberto* — criado por **korscripterdev**

Cliderness é um sandbox de sobrevivência livre desenhado apenas com caracteres CP437 em 16 cores VGA numa grade de 80×25. Um único motor ISO C90 roda em MS-DOS, FreeDOS, Windows, Linux, macOS e outros Unix. Sem história e sem final: explore, minere, construa, transforme o terreno e sobreviva.

## O que funciona na 1.1.0

* Mundo 3D determinístico a partir de uma semente (idêntico em 16 e 64 bits), 46 biomas, cavernas, minérios, oceanos, rios e lagos subterrâneos; só os chunks alterados são salvos.
* Andar, correr, agachar, nadar, minerar, colocar, ferramentas com durabilidade; saúde, fôlego, fome, sede, cansaço, temperatura corporal, ferimentos e doenças.
* Fabricação (87 receitas), bancada, fornalha, fogueira, bigorna e altar, plantações e árvores, baús, camas, equipamento e mochilas, reparos.
* ClideFluid: água e lava que fluem conservando o volume, represas, poças de chuva, baldes; minas abandonadas, ruínas e cidades ancestrais.
* 30 criaturas (animais, monstros, PNJ) com IA por objetivos, visão e audição, rotinas de dia e de noite; combate corpo a corpo e com arco, armaduras, saque, comércio, 4 dificuldades.
* Barcos: jangada, barco a remo, veleiro e navio mercante; remos e velas com vento, porões, flutuação e sobrecarga, 3 estágios de dano no casco, alagamento, naufrágio.
* A Dimensão Ancestral: portais, 8 biomas (com fendas do vazio), minérios, plantas e criaturas ancestrais, o altar ancestral.
* Estações e fases da lua, 21 tipos de clima, visão do céu, som procedural (PCM ao vivo no Windows, Linux e macOS; alto-falante do PC no DOS).
* Novo na 1.1.0: busca de caminho A* para as criaturas, atributos que crescem jogando, pesca e furtividade, deriva com o vento e tripulações contratadas, a segunda lua e o mundo com anéis do céu antigo, áudio mixado em streaming no Linux, e ajuda e nomes nos 11 idiomas.
* **240** comandos, ajuda offline com 63 tópicos em 5 níveis, interface em 11 idiomas, salvamento seguro (arquivo temporário + CRC).

Limites: os executáveis de DOS são compilados e ligados na CI (Open Watcom, 16 bits e DOS/4GW), mas ainda não foram executados em hardware real nem em emulador. O áudio ao vivo no Linux e no macOS não foi testado em dispositivos reais. Detalhes em [ROADMAP.md](ROADMAP.md).

## Compilar

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
make && make test
sh builds/dos/build_watcom.sh 16
python tools/package.py            # ZIP / tar.gz (CPack)
```

Execute `build/cliderness` (ele procura `data/` ao lado do executável ou na pasta atual). `--help` mostra as opções, `--version` a versão e o autor, `--play-sfx rain` testa o dispositivo de áudio.

## Controles (padrão)

`WASD` mover · `Shift` correr · `Ctrl`/`X` agachar · `F` atacar/minerar · `R` colocar · `>`/`<` descer/subir · `E` pegar/embarcar/negociar · `U` usar · `K` fabricar · `Tab` inventário · `C` personagem · `Shift direito`/`V` céu · `F1`/`?` ajuda · `Esc` pausa · `/` console. Todas as teclas podem ser alteradas.

## Documentação

[DESIGN.md](DESIGN.md) · [ARCHITECTURE.md](ARCHITECTURE.md) · [ROADMAP.md](ROADMAP.md) · [Alterações](changelogs.pt.md) · [commands](docs/commands/commands.md) · [DOS](docs/platforms/dos.md)

© 2026 korscripterdev · GPL-3.0-or-later · Veja [LICENSE](LICENSE).
