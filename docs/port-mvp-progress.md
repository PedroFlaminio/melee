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
| Luta (fighters, stage, colisão, câmera, HUD, KO) | `GS_VS` está na tabela do host: a entrada passa por refração, efeitos, itens, estágio e câmera, cria os dois Fox, carrega pausa, HUD e flash de fundo e entra no laço de frames; os scripts de comando despacham pelo opcode certo, a luta roda sem erro, a pausa responde ao START e L+R+A+START encerra a luta como no contest pelo código do jogo, com `OnExit` e volta à CSS; com `coll_data` os lutadores pousam no estágio e a câmera fica nele; os dois Fox aparecem no tamanho certo sobre o estágio, com uma faixa preta grande ainda sem causa; faltam a resposta ao stick, KO e tela de resultados | 30% |
| Áudio, distribuição e regressão end-to-end | parcial; a rota título → menu → CSS → SSS → luta → CSS → menu é um teste, sem imagem nem áudio conferidos | 10% |

### Evidências verificadas

- `ctest --preset host-debug`: 15/15 testes aprovados; 194/194 testes
  unitários.
- `ctest --preset host-sanitize -V`: 15/15 e 194/194, sem erro do ASan; a rota
  VS com a luta leva 127,1 s. O UBSan só imprime (17 pontos distintos, listados
  em `native_port_status.md`): chamadas por ponteiro de função de outro tipo,
  `1 << 31` em `int` e duas leituras além de vetor que caem na mesma struct.
- A cena de título, animações e a transição para o menu principal possuem testes
  com assets locais.
- `melee-host-vs-match-asset`: título (122 frames) → menu (120) → CSS (141) →
  SSS (149) → luta (175) → CSS (56) → menu, com dois pads roteirizados. As duas
  portas abrem como HMN, os dois jogadores escolhem Fox, START leva à SSS e o
  cursor escolhe Hyrule Temple (estágio 14 com Fox nos slots 0 e 1). Na luta, o
  pad 1 pausa depois que o HUD liga (frame 655) e sai com L+R+A+START; o modo
  volta à CSS, e B segurado leva ao menu. 27,3 s no `host-debug`.
- `--diagnose-local-match` materializa dados de duas pessoas, regras e estágio,
  mas ainda não inicia a cena de combate.

## Próximo marco

Conferir o que a luta Fox vs. Fox em Hyrule Temple (`GS_VS`) faz entre a
entrada e a saída. A rota já entra na luta, roda os frames sem erro e sai pelo
menu de pausa, e isso é teste. A imagem dos frames já sai por
`FRAME:BMP=arquivo`, com estágio, HUD, "Ready" e contagem. Com `coll_data` os lutadores pousam e a
câmera fica no estágio. Os dois Fox já aparecem no tamanho certo
sobre o estágio. O próximo passo é a faixa preta grande que cobre parte do
estágio (a sombra projetada, cuja textura vem de uma cópia de framebuffer que
o host ainda não produz, é a suspeita) e depois ver os lutadores responderem
ao stick e aos botões (a pausa só prova que a entrada chega à cena). Sobram os relatos do UBSan, que a rota da luta multiplicou, e dois
casos de layout conhecidos fora da rota de VS (`gm_1832.c`, `gm_19EF.c`). Do
estágio faltam `itemdata`, `ALDYakuAll`, `yakumono_param`, `map_plit` e
`quake_model_set`, e do modo VS a tela de resultados. Hyrule Temple segue
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
| 2026-09-14 | 62% | O NaN do primeiro frame era da câmera: `Camera_ApplyQuake` lia a descrição da câmera pelo layout de statics em sequência do console e gravava NaN na translação de tremor. No host a função lê `cm_803BCB64` direto. O primeiro frame desenha, e a luta segue até os procs de um frame seguinte, onde o sistema de partículas guarda endereços de gerador em `u32`. |
| 2026-09-14 | 64% | Lote de correções de layout de 32 bits achadas pela rota da luta sob ASan: endereços de gerador de partícula em `u32`, a visão `UnkX` do `IfDamageState`, structs lidas sobre statics em sequência (`lbrefract.c`, `ftmaterial.c`, `ft_800852B0`), segmentos de colisão e o pool de `HSD_psAppSRT` alocados com o tamanho do console, a posição da luz do lutador em floats, a matriz 3x4 de `lbVector_WorldToScreen` e a cor do HUD convertida para `s8`. As listas de símbolos de `lbarchive.c`, terminadas num `0` que o x86-64 passa com a metade alta indefinida, viram vetores de ponteiros. Todos os 13 arquivos pré-processam idênticos sem `MELEE_HOST`. Com `GS_VS` só local, a luta entra no laço de frames; sob ASan os lutadores chegam aos scripts de comando da animação e param em `Command_04` (`lbcommand.c:57`), no `RebirthWait`, com leitura de endereço inválido. |
| 2026-09-14 | 65% | O SEGV em `Command_04` era o opcode lido dos bits errados: `ftAction_80073240` despacha por `gmScriptEventDefault` (`ft/types.h`), bit-fields fora de `lb/types.h` que o gerador de layouts não cobre, e o host tirava o opcode dos seis bits baixos da palavra. Com os campos invertidos sob `MELEE_HOST` (teste unitário; `ftaction.c` pré-processa idêntico sem o define), a luta com `GS_VS` só local roda o laço de frames sem erro: 600 frames da luta em ~60 s no `host-debug` e 400 s sob ASan, sem terminar sozinha. Sem a entrada: 194/194 e ctest 15/15 nos dois presets, rota VS em 20,0 s sob ASan. |
| 2026-09-14 | 68% | `GS_VS` entra na tabela do host e a rota VS atravessa a luta pelo código do jogo: Fox vs. Fox em Hyrule Temple roda 175 frames, o HUD libera a pausa no frame 655, START na porta 1 pausa e L+R+A+START encerra a luta como no contest; `gm_Scene_Vs_OnExit` monta o resultado, o modo volta à CSS (o host não tem a tela de resultados) e B segurado leva ao menu. Teste `melee-host-vs-match-asset` no lugar de `melee-host-vs-selection-asset`: 27,3 s no `host-debug`, 127,1 s sob ASan, sem erro do ASan. 194/194 e ctest 15/15 nos dois presets. |
| 2026-09-14 | 70% | Primeira imagem da luta: `--run-modes` aceita `FRAME:BMP=arquivo` e desenha aquele frame pelo presenter escondido. Na rota do teste, o frame 640 mostra Hyrule Temple, o "Go!", o cronômetro em 02:00, P1 e P2 com 0% e o emblema da Star Fox; o 675 tem o cronômetro em 01:59.69; o 695 mostra o menu de pausa do P1 com a legenda L R A START. Problemas vistos: no 560 o letreiro de início sai como quadriláteros brancos, no mesmo frame em que uma textura C8 não decodifica ("TLUT index exceeds palette"); a câmera fica colada na parte de baixo do estágio e os lutadores não aparecem; o frame 400 (SSS) sai quase todo azul. 194/194 e ctest 15/15 nos dois presets; teste da luta em 26,9 s no `host-debug` e 123,4 s sob ASan, sem erro do ASan. |
| 2026-09-14 | 71% | Os quadriláteros brancos eram paleta errada: o frame é lido depois do último draw, e a textura pedia a paleta pelo nome (`GX_TLUT0`), que já guardava a de outro draw. O recorder GX passa a guardar a paleta de cada draw junto da textura capturada, e o decodificador deixa o padding dos blocos fora da paleta (dois testes unitários). Os frames 560 e 600 mostram o letreiro "Ready", a contagem, o céu e os estandartes de Hyrule Temple, sem textura recusada nos cinco frames capturados. A câmera segue colada no estágio, com os lutadores fora do quadro. 196/196 e ctest 15/15 nos dois presets; teste da luta em 26,8 s no `host-debug` e 125,3 s sob ASan, sem erro do ASan. |
| 2026-09-14 | 73% | `coll_data` traduzido (71 de 71 estágios, com o layout levantado antes nos arquivos). Sem ele o jogo usava um mapa de colisão vazio, e os lutadores caíam depois do Ready com a câmera atrás deles (medido sob gdb: y de 22 a -201 entre os frames 340 e 440 do modo). Com a colisão os dois pousam em Hyrule Temple e a câmera fica no estágio. O pouso achou dois pontos, corrigidos sob `MELEE_HOST`: `fn_8001E60C` terminava uma lista de FObj por um ponteiro nunca atribuído quando a parte só tem trilhas de translação (SIGSEGV), e `fn_8002113C` copiava um quaternion num `Vec3` da pilha (ASan). Nos frames 640 e 675 o estágio aparece inteiro, mas os lutadores não são desenhados. 197/197 e ctest 15/15 nos dois presets; teste da luta em 27,3 s no `host-debug` e 124,6 s sob ASan, sem erro do ASan; 21 pontos do UBSan. |
| 2026-09-14 | 74% | Os lutadores passam a ser desenhados. `ftDrawCommon_800805C8` só desenha o corpo com `x21FC_flag.b7`, e `fighter.c:747` liga essa flag gravando `byte = 1` no union `UnkFlagStruct`. No console esse byte é o bit `b7`, porque o MWCC aloca bit-fields a partir do bit mais alto; no host ele ligava `b0`. Sob `MELEE_HOST` o union declara os bits na ordem inversa. A captura mais que dobra (28.459 triângulos no frame 560), mas os lutadores saem como planos enormes de cor chapada que tomam a tela e escondem o estágio. 197/197 e ctest 15/15 nos dois presets; teste da luta em 56,6 s no `host-debug` e 260,0 s sob ASan, sem erro do ASan; 24 pontos do UBSan. |
| 2026-09-14 | 76% | Os dois Fox aparecem no tamanho certo sobre Hyrule Temple. Os planos gigantes eram vértices sem a translação da câmera: o recorder GX do host guardava a matriz de normal nas mesmas linhas da matriz de posição, e o HSD carrega a inversa transposta, que não tem translação, logo depois da posição de todo PObj iluminado. O host passou a guardar as matrizes de normal à parte, como o GX (teste unitário). O esqueleto já estava certo (medido sob gdb). Cada `FRAME:BMP=` imprime o relatório da captura, com views e sequências de draw. Resta uma faixa preta grande sobre o estágio, sem causa medida. 198/198 e ctest 15/15 nos dois presets; teste da luta em 56,7 s no `host-debug` e 260,3 s sob ASan, sem erro do ASan; 24 pontos do UBSan. |
