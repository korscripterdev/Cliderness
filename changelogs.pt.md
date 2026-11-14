# Registro de alterações

[English](changelogs.md) · [한국어](changelogs.ko.md) · [日本語](changelogs.ja.md) · [简体中文](changelogs.zh-CN.md) · [繁體中文](changelogs.zh-TW.md) · [Español](changelogs.es.md) · [Français](changelogs.fr.md) · [Deutsch](changelogs.de.md) · **Português** · [Русский](changelogs.ru.md) · [Italiano](changelogs.it.md)

Mudanças importantes do Cliderness (criado por korscripterdev).

## [1.1.0] — 2026-11-15

Completa o que a 1.0.0 deixou em aberto no roteiro.

### Adicionado
- Busca de caminho A* para as criaturas (heap binário e hash, sobe 1 e desce 3 blocos, guarda uma rota de 12 passos).
- Atributos que crescem jogando até 30: cada habilidade treina um atributo; Constituição sobe ao sofrer dano (+2 de vida máxima), Vontade ao suportar extremos, Sorte com saque e críticos.
- Pesca: vara de pescar, fisgada após 3-8 segundos, chance 35% + Pesca / 2 + Sorte / 4, relíquias na Dimensão Antiga.
- Habilidade de furtividade: passar agachado perto de criaturas acordadas a treina e deixa você mais silencioso.
- Navios derivam com ventos acima de 25%, a menos que estejam ancorados ou encostados na margem.
- O Marinheiro (costas) vende contratos de tripulante; a tripulação rema sem vigor (+10% cada), esgota 2 l por segundo e pesa 75 kg. Beliches: jangada 0, barco a remo 1, veleiro 2, mercante 4.
- O céu antigo: a segunda lua Veyra (ciclo de 3 dias), um mundo com anéis, o véu de éter, estrelas magenta e ciano.
- O áudio no Linux/BSD transmite PCM mixado com 60 ms de antecedência.
- Ajuda nos 11 idiomas (63 tópicos) e nomes de blocos, itens, biomas, clima e efeitos.

### Alterado
- As versões de DOS compilam e ligam na CI (Open Watcom 16 bits e DOS/4GW); ainda não executadas em hardware nem emulador.
- Tabelas de idioma no DOS: 36000 bytes e 960 entradas.

## [1.0.0] — 2026-11-15

Fase 10 do roteiro (sistemas avançados). As dez fases estão implementadas; o que falta verificar está em ROADMAP.md.

### Adicionado
- Áudio PCM ao vivo: Windows waveOut (mixado com o que toca), Linux/BSD via PulseAudio, PipeWire ou ALSA, macOS via afplay. Tudo é sintetizado pelo DSP procedural.
- Passos, trovões e som ambiente (chuva, vento, tempestade, cavernas, Dimensão Ancestral, pássaros, corvos, barcos). `--play-sfx` testa o dispositivo.
- 119 comandos novos (240 no total): formas de construção, informações, ações iguais às teclas, armazenamento, pontos de referência, ponto de morte, interface e sistema.
- Pacotes: regras install do CMake e CPack (ZIP, tar.gz), `tools/package.py` (com ZIP para DOS).

## [0.6.0] — 2026-11-14

Fase 9 do roteiro (a Dimensão Ancestral).

### Adicionado
- Portais: fabrique um núcleo de portal na bigorna, coloque-o e pressione E para viajar; um portal de retorno é construído.
- A Dimensão Ancestral: semente própria, 8 biomas (com fendas do vazio), blocos e plantas ancestrais, veios de éter e minério de metal estelar.
- 7 criaturas ancestrais, cidades e santuários ancestrais, o altar ancestral com 9 receitas, relíquias.
- Clima da dimensão (chuva de estrelas, névoa silenciosa, vento de éter), renascimento por dimensão. 5 comandos novos (121 no total).

## [0.5.0] — 2026-11-14

Fase 8 do roteiro (barcos).

### Adicionado
- Jangada, barco a remo, veleiro e navio mercante (5x3 células) com receitas.
- R para lançar, E para embarcar/desembarcar, F para recolher um barco vazio. Pilotagem com as teclas de movimento (potência -1..3, curva), âncora.
- Remos gastam fôlego; velas usam a direção e a força do vento. Habilidade de navegação.
- Flutuação e capacidade, porões, sobrecarga. 3 estágios de dano no casco, vazamentos, alagamento, retirada de água, reparo, ondas, encalhe, naufrágio.
- Sons de rangido e rachadura. 12 comandos novos (116 no total).

## [0.4.0] — 2026-11-14

Fase 7 do roteiro (criaturas e combate).

### Adicionado
- 22 criaturas definidas por dados: 11 animais, 8 monstros e 3 PNJ.
- IA por objetivos (sobreviver, atacar, abrigar-se, voltar para casa, descansar, vagar) com visão, audição, clima, escuridão e furtividade.
- Surgimento natural por bioma, hora, cavernas e água; desaparecem longe do jogador.
- Combate: F ataca, espadas, lança, arco e flechas, críticos, armadura, sangramento e veneno. Mortos-vivos queimam à luz do dia.
- 4 níveis de dificuldade; criaturas, abates e dificuldade são salvos. Saque, carne cozida, armadura de couro, farinha de osso, pederneira, 15 receitas.
- Comércio com PNJ (E). 11 comandos novos (104 no total).

## [0.3.0] — 2026-11-14

Fase 6 do roteiro (simulação do mundo).

### Adicionado
- ClideFluid: água e lava que fluem com níveis 1-15, conservação do volume, mar e lagos que se reabastecem, obsidiana, poças de chuva.
- Baldes, rios nas terras baixas e lagos subterrâneos.
- Estruturas: minas abandonadas e ruínas de pedra com caixas de saque.
- 7 comandos novos (93 no total).

### Alterado
- Rios, lagos e estruturas mudam o terreno não explorado de mundos antigos.

## [0.2.0] — 2026-11-14

Fase 4 do roteiro (artesanato e construção) e o que faltava da fase 3.

### Adicionado
- Tela de artesanato (K), 58 receitas em dados, bancada, fornalha, fogueira, bigorna e combustível.
- Fundição, culinária e agricultura (enxadas, terra arada, trigo e batatas, mudas que viram árvores).
- Baús, camas e sono, espaços de equipamento (cabeça, corpo, pernas, pés, costas) e reparo.
- Efeitos: sangramento, fratura, intoxicação, resfriado, molhado, bem alimentado, descansado e remédios.
- 15 comandos novos (86 no total). Licença: GPL-3.0 ou posterior.

### Alterado
- Descrições de comandos novos aparecem em inglês (a interface continua traduzida em 11 idiomas).

## [0.1.0] — 2026-11-14

Primeira base jogável (fases 1–3 completas; 4–6 e 10 parciais).

### Adicionado
- Motor ISO C90 com adaptadores para console do Windows, terminais POSIX, MS-DOS/FreeDOS e um adaptador sem tela para testes.
- Framebuffer CP437 / VGA-16 com saída de células alteradas e caracteres largos.
- Geração inteira por semente, 38 biomas, cavernas, minérios, oceanos e armazenamento seguro de chunks.
- Movimento, mineração, colocação, ferramentas e durabilidade, habilidades, sobrevivência, temperatura, morte e renascimento.
- Inventário, barra rápida e itens com flutuabilidade.
- ClideTime, 18 climas, visão do céu; ClideConsole com 71 comandos.
- Ajuda de 45 tópicos em inglês e coreano, interface em 11 idiomas, DSP procedural com exportação WAV.
- Compilação com CMake, Make, Open Watcom e DJGPP; GitHub Actions; 293.610 verificações.

### Limitações conhecidas
- Ainda sem artesanato, criaturas, combate, água corrente, navios ou Dimensão Antiga.
- Sem áudio PCM ao vivo no Windows/Linux/macOS (roda em silêncio).
- As versões DOS ainda não foram compiladas nem executadas (sem ferramentas locais).
- A ajuda existe só em inglês e coreano; os outros idiomas usam a ajuda em inglês.
