# Progresso do MVP do port nativo

## Definição de MVP

Fluxo local completo: iniciar o executável, navegar de título para VS, escolher
dois jogadores e um estágio, e concluir uma luta local com vídeo, entrada,
áudio e resultado funcional.

## Linha de base — 13 de setembro de 2026

**Estimativa: 40% (faixa de confiança: 35–45%).**

| Área | Estado | Peso no MVP |
| --- | --- | --- |
| Plataforma host (memória, relógio, DVD virtual, input) | funcional e testada; roteiro de entrada com stick e quatro portas | 15% |
| Assets e renderização HSD/GX | funcional para cenas/modelos selecionados | 20% |
| Inicialização, título e menu principal | título e rota até o menu validados | 15% |
| Configuração de VS | CSS e SSS executadas com dois pads; seleção validada, imagem ainda não conferida | 10% |
| Luta (fighters, stage, colisão, câmera, HUD, KO) | a cena liga; com `GS_VS` só local, a entrada passa por refração e efeitos e para nos dados de jogador | 30% |
| Áudio, distribuição e regressão end-to-end | parcial; sem partida validada | 10% |

### Evidências verificadas

- `ctest --preset host-debug`: 14/14 testes aprovados; 182/182 testes
  unitários.
- `ctest --preset host-sanitize -V`: 14/14 e 182/182, sem erro do ASan; a rota
  VS leva 19,8 s. O UBSan só relata chamadas por ponteiro de função de outro
  tipo (quatro pontos, listados em `native_port_status.md`).
- A cena de título, animações e a transição para o menu principal possuem testes
  com assets locais.
- `melee-host-vs-selection-asset`: título (122 frames) → menu (120) → CSS
  (141) → SSS (149), com dois pads roteirizados. As duas portas abrem como HMN,
  os dois jogadores escolhem Fox, START leva à SSS e o cursor escolhe Hyrule
  Temple. A seleção lida de volta do modo VS é estágio 14 com Fox (2) nos slots
  0 e 1, e o modo para com nome na cena de luta (`GS_VS`, 0x02).
- `--diagnose-local-match` materializa dados de duas pessoas, regras e estágio,
  mas ainda não inicia a cena de combate.

## Próximo marco

Entrar na cena de luta (`GS_VS`) com a seleção que a rota já produz: Fox vs.
Fox em Hyrule Temple, estágio escolhido por estar liberado sem cartão de
memória e por ter o menor módulo (`grshrine.c`). Final Destination e
Battlefield ficam travados na SSS sem dados salvos.

## Registro de atualizações

| Data | Percentual | Mudança/evidência |
| --- | ---: | --- |
| 2026-09-13 | 25% | Linha de base criada; infraestrutura, título e menu parcial validados. |
| 2026-09-13 | 25% | UI de memory card isolada no host; rota validada até `GM_VS (0x02)`. |
| 2026-09-13 | 25% | CSS, SSS e regras de menu passaram a compilar/linkar no core host; falta conectá-los e executá-los. |
| 2026-09-13 | 25% | `GM_VS`, CSS e SSS registrados na tabela host. CSS inicia e para explicitamente no tradutor ausente de `MnSelectChrDataTable`; não há crash silencioso. |
| 2026-09-13 | 30% | `MnSelectChrDataTable` passou a materializar câmera, luzes, fog e nove modelos CSS no layout host; carga verificada com `MnSlChr.usd`. |
| 2026-09-13 | 35% | `MnSelectStageDataTable` passou a materializar câmera, luzes, fog, 11 modelos e o modelo aleatório SSS; carga verificada com `MnSlMap.usd`. |
| 2026-09-13 | 35% | CSS parava em `sislib.c:95` ("Memory Empty"): o pool SIS de 0x2400 bytes da cena é medido para o PowerPC. Pool dobrado e blocos alinhados a ponteiro sob `MELEE_HOST`; a CSS passa a rodar. |
| 2026-09-13 | 40% | CSS e SSS executadas com entrada de dois pads (stick e portas no roteiro de `--run-modes`); seleção Fox/Fox em Hyrule Temple validada; cena ausente (`GS_VS`) para com nome. Teste `melee-host-vs-selection-asset`. |
| 2026-09-13 | 40% | `host-sanitize` voltou a ligar (`gm_80177724` parado com nome). A rota VS sob ASan achou dois bugs de host: leitura além da tabela de estágios da SSS (agora com os bytes que o console lê) e shape animation lendo vértices big-endian como nativos (`pobj.c`), que corrompia todo modelo com shape animation. |
| 2026-09-13 | 40% | Sondagem da luta (`GS_VS` na tabela, sem commit): primeira onda de link com 437 símbolos indefinidos em ~75 arquivos, a maior parte do núcleo de lutador. Inventário em `fight_flow_port.md`. |
| 2026-09-13 | 40% | `host-sanitize` sem exclusões de instrumentação (dead stripping dos globais do ASan e `-z start-stop-gc`). Instrumentar os 30 arquivos antes excluídos achou um estouro de pilha no menu, leituras fora de vetor e deslocamentos indefinidos, corrigidos. 741 dos 808 `.c` fora do core já compilam. |
| 2026-09-13 | 42% | Toda a decomp de `src/melee` compila no host e está no core (808 arquivos, mais o sistema de partículas do baselib). As rotas existentes não mudaram; a luta pode ser ligada sem acrescentar fontes. `atan2f`, `acosf` e `asinf` passaram a ser as do jogo, no lugar das da glibc. |
| 2026-09-13 | 45% | A cena de luta (`GS_VS`) liga sem símbolo indefinido (partículas, textura indireta e offsets de textura no GX do host). Com a cena na tabela, a rota entra na luta e para no primeiro dado sem tradutor, `lbRefData`. |
| 2026-09-13 | 45% | `lbRefData` traduzido, com teste unitário. A entrada da luta passa pela refração e para nos efeitos: `effCommonDataTable` e os bancos de partícula, que relocam ponteiros de 32 bits no lugar e pedem um loader próprio do host. |
| 2026-09-14 | 46% | Loader de partículas do host: `eff*DataTable` e `map_ptcl`/`map_texg` viram bancos já localizados em largura de ponteiro, com bytes de comando, texels e paletas verbatim; os 76 símbolos de partícula do disco traduzem. Joints de spline traduzidos. A entrada da luta passa pelos efeitos e para em `plLoadCommonData` (`PdPm.dat`). |
