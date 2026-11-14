# Registro de alterações

[English](changelogs.md) · [한국어](changelogs.ko.md) · [日本語](changelogs.ja.md) · [简体中文](changelogs.zh-CN.md) · [繁體中文](changelogs.zh-TW.md) · [Español](changelogs.es.md) · [Français](changelogs.fr.md) · [Deutsch](changelogs.de.md) · **Português** · [Русский](changelogs.ru.md) · [Italiano](changelogs.it.md)

Mudanças importantes do Cliderness (criado por korscripterdev).

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
