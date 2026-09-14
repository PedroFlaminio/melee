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
| Luta (fighters, stage, colisão, câmera, HUD, KO) | a cena liga; com `GS_VS` só local, a entrada passa por refração, efeitos, itens, estágio e câmera, cria os dois Fox (dados, fantasia, animações e posição inicial), carrega o menu de pausa, o HUD e o flash de fundo, termina a entrada da cena e roda os procs do primeiro frame; para no desenho, ao projetar a caixa de câmera de um lutador com posição fora da faixa (`lbvector.c:383`) | 30% |
| Áudio, distribuição e regressão end-to-end | parcial; sem partida validada | 10% |

### Evidências verificadas

- `ctest --preset host-debug`: 15/15 testes aprovados; 193/193 testes
  unitários.
- `ctest --preset host-sanitize -V`: 15/15 e 193/193, sem erro do ASan; a rota
  VS leva 20,3 s. O UBSan só relata chamadas por ponteiro de função de outro
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

Seguir a entrada da cena de luta (`GS_VS`) até o primeiro frame da luta Fox
vs. Fox em Hyrule Temple. Os dois lutadores são criados e a montagem da cena
(`fn_8016E730`) termina, com pausa, HUD e flash de fundo; a cena entra no laço
de frames. O próximo passo é o desenho do primeiro frame, que para ao projetar
a caixa de câmera de um lutador com posição fora da faixa, e depois os frames
seguintes. Do estágio faltam `coll_data` e os outros dados que o jogo
hoje substitui por faixas padrão. Hyrule Temple segue
como alvo por estar liberado sem cartão de memória e ter o menor módulo
(`grshrine.c`); Final Destination e Battlefield ficam travados na SSS sem dados
salvos.

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
| 2026-09-14 | 46% | `plLoadCommonData` (limiares de bônus e truques) traduzido, com teste unitário. A entrada da luta passa pelos dados de jogador e pelo começo do estágio e cai em `Ground_801C0754`: a entrada de Hyrule Temple em `stage_datas` é nula, porque o host liga os estágios por referência fraca. |
| 2026-09-14 | 46% | A tabela de estágios de `ground.c` liga forte (sem `host_weak_stages.h`), sem símbolo indefinido e com as rotas nos mesmos frames. A entrada da luta acha Hyrule Temple, lê `GrSh.dat` e para nos dados de estágio, que o host ainda não traduz (`map_head`, `coll_data`, `grGroundParam` e mais cinco). |
| 2026-09-14 | 47% | `grGroundParam` (parâmetros e linhas `StageParam` de cada estágio) traduzido, com teste; os 71 `Gr*.dat` traduzem. `--load-archive` e `--sweep-archives` passam a registrar os tradutores C do jogo (varredura: `game_data` 170/170). A entrada da luta passa por `Ground_801C28CC` e para no display de troféus, que pede as sete tabelas de `TyDatai.usd`. |
| 2026-09-14 | 48% | As sete tabelas de troféu de `TyDatai.usd` traduzidas (vetores de structs de escalares terminados por -1), com teste. A entrada do estágio termina (`Stage_802251E8` retorna) e a luta para dois passos adiante, nos dados comuns de item (`itPublicData`, `ItCo.usd`). |
| 2026-09-14 | 48% | Levantamento de `itPublicData`: 43 itens comuns, 8 dos 118 de personagem e 47 Pokémon em `ItCo.usd`, 11 blocos de atributos próprios com ponteiros e scripts de estado lidos por bit-fields sobre `u32`, o mesmo formato dos scripts de lutador. Plano em `fight_flow_port.md`. |
| 2026-09-14 | 49% | Scripts de comando no host: a API de arquivo converte as palavras dos scripts para a ordem nativa, as structs de bit-field do host são geradas de `lb/types.h` com os campos invertidos (255 campos conferidos contra o modelo do MWCC) e sub-rotina e goto usam distâncias relativas. Vale para itens, lutadores e sobreposição de cor; nos 150 scripts de estado de `ItCo.usd` a regra de parada vale. |
| 2026-09-14 | 51% | `itPublicData` traduzido: dados comuns, 98 `Article` (43 comuns, 8 de personagem, 47 Pokémon) com atributos, hurtboxes, estados com animações e scripts, modelos e a tabela de cor, mais as restrições RObj de bytecode dos modelos. Atributos próprios por tipo de item e dinâmica ficam de fora, com parada nomeada ao criar o item. A entrada da luta passa pelos itens e pelo áudio e para no `map_head` do estágio. |
| 2026-09-14 | 53% | `map_head` traduzido (69 de 71 estágios): modelos com câmera, luzes e fog, pares, splines, overrides de luz com o mesmo `HSD_LightDesc*` das luzes dos modelos (a contagem no disco é o dobro da tabela, e o jogo lê além dela) e os materiais dos modelos. A entrada da luta passa pelo estágio e pela câmera e para nos dados comuns de lutador (`ftLoadCommonData`, `PlCo.dat`). |
| 2026-09-14 | 55% | `ftLoadCommonData` traduzido: as 23 tabelas comuns de lutador (`ftCommonData`, partes por tipo, scripts de cor, tremor, modificadores, `CrowdConfig` e as tabelas da IA de CPU), com teste. A entrada da luta passa pela inicialização dos jogadores e para em `Fighter_Create`, nos dados do Fox (`ftDataFox`, `PlFx.dat`). |
| 2026-09-14 | 55% | Levantamento de `ftData*`: os 24 campos de `ftDataFox`, tabelas de ação cujos 10.091 scripts (58 arquivos de personagem) obedecem à conversão de comandos fora das cópias do Kirby, atributos próprios por personagem, itens, dinâmica e hurtboxes, e `x54`, declarado `int` mas ponteiro no disco. Plano em `fight_flow_port.md`. |
| 2026-09-14 | 58% | `ftDataFox` traduzido (atributos, ações, partes, dinâmica, hurtboxes, itens, SFX e IK), com teste. Endereços de animação em largura de ponteiro; fila de ARAM (`lbarq.c`) montada no boot, com a espera síncrona dando passos no escalonador; e, no `map_head`, o joint de cada entrada de pares, sem o qual os lutadores nasciam na posição lida da pilha. A entrada da luta cria os dois Fox e para no menu de pausa (`ScGamPause_scene_data`, `GmPause.dat`). |
| 2026-09-14 | 59% | `_scene_data` na API de arquivo: `SceneDesc` com modelos, câmeras, luzes e fogs e suas animações; câmeras e fogs são vetores sem terminador, contados pelas entradas com descritor até a próxima fronteira. Os 43 do disco traduzem e carregam (1.495 objetos), com teste. A entrada da luta carrega o menu de pausa e a cena do HUD e para nos modelos do HUD (`ScInfCnt_scene_models`). |
| 2026-09-14 | 60% | `_scene_models` na API de arquivo e as tabelas do HUD sem sufixo (`Stc_scemdls`, `Stc_rarwmdls`, `tdsce`, `lupe`), com teste; os 26 do disco carregam (268 JObjs). `lbBgFlashColAnimData` traduzido. A montagem da cena de luta (`fn_8016E730`) termina; a entrada para nos dígitos de dano do HUD, onde `ifStatus_802F6194` anda por um JObj com o layout de GObj. |
| 2026-09-14 | 61% | `ifStatus_802F6194` anda pelo próprio JObj no host (no console `next_gx` e `next` do GObj caem sobre `child` e `next` do JObj). A entrada da cena de luta termina e a cena entra no laço de frames: o primeiro frame roda os procs e para no desenho, ao projetar a caixa de câmera de um lutador com posição fora da faixa (`lbvector.c:383`). |
