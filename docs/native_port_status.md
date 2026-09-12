# Status do port nativo

Atualizado em 12 de setembro de 2026.

## Concluido

- [x] Plano de projeto e criterios de aceite.
- [x] Build CMake/Ninja separado do build matching.
- [x] C ABI inicial da camada host.
- [x] Tipos host de largura fixa.
- [x] Relogio monotônico e facade inicial de tempo Dolphin.
- [x] Facade por thread para secoes criticas de interrupcao Dolphin.
- [x] Heap host alinhado a 32 bytes conectado a `HSD_MemAlloc/HSD_Free`.
- [x] Allocator de objetos e listas HSD originais executando com enderecos
  nativos de 64 bits.
- [x] Bootstrap headless idempotente da baselib com pools de listas, vetores,
  matrizes, AObj/FObj e tabela de IDs originais.
- [x] Canais de animacao FObj originais com carga, encadeamento, allocator e
  interpolacao Hermite portavel.
- [x] Controladores AObj originais com ownership de FObj, flags e requisicao de
  animacao; referencias JObj inseguras sao rejeitadas no host.
- [x] Compatibilidade C portavel para as operacoes de matriz/vetor paired-single
  usadas pelo HSD no lugar do assembly PowerPC.
- [x] Scheduler deterministico ordenado por tick e sequencia.
- [x] Snapshot de entrada deterministico para quatro controles.
- [x] Adaptador `PADRead` para o snapshot do host (sem backend de janela).
- [x] Calibracao original `PADClamp` compilada e testada no host.
- [x] Executavel de diagnostico x86-64.
- [x] Inventario automatizado de bloqueadores de portabilidade.
- [x] Parser de ISO/GCM e FST com validacao de limites e paths.
- [x] Verificacao de `GALE01` pelo SHA-1 do `main.dol`.
- [x] Extracao atomica com manifesto e entry numbers de DVD.
- [x] Indice DVD runtime e leitura de recursos pela C ABI host.
- [x] Facade DVD sincrona (`DVDOpen`/`DVDFastOpen`/`DVDReadPrio`) sobre assets
  extraidos.
- [x] Leitura e seek DVD assincronos entregues em ordem pelo scheduler host.
- [x] Fila original `HSD_DevComRequest` conectada ao DVD host em teste
  integrado.
- [x] Parser HSD big-endian baseado em offsets, sem truncar ponteiros.
- [x] Enumeracao de roots publicos HSD para diagnostico de assets reais.
- [x] Grafo runtime HSD seguro em 64 bits: referencias internas preservadas
  como offsets validados, sem relocacao in-place.
- [x] Leitores runtime HSD para campos big-endian e referencias relocadas.
- [x] Primeiro schema HSD tipado: `dbLoadCommonData` e suas tres tabelas.
- [x] Leitura segura de strings HSD; primeiras entradas reais das tres tabelas
  de `DbCo.dat` decodificadas.
- [x] Materializacao completa das tabelas de nomes de `DbCo.dat` com bounds e
  relocacoes validados.
- [x] `DbCo.dat` real carregado em x86-64: root `dbLoadCommonData` e 854
  referencias internas validadas.
- [x] Recorder GX host para comandos de vertice sem acesso a MMIO.
- [x] Primeiro `GX_TRIANGLES` montado em vertices runtime no backend headless.
- [x] Conversao headless de `GX_TRIANGLESTRIP`, `GX_TRIANGLEFAN` e `GX_QUADS`
  para listas de triangulos.
- [x] Captura de vertices GX diretos com position, normal, RGBA e UV.
- [x] Estado GX VCD/VAT para oito formatos, arrays com stride e indices de
  8/16 bits para position, normal, cor e UV principal.
- [x] Decodificacao big-endian de componentes U8/S8/U16/S16/F32 com ponto fixo
  e cores RGB565/RGB8/RGBX8/RGBA4/RGBA6/RGBA8.
- [x] `GXCallDisplayList` host interpreta comandos PObj, formatos VAT, atributos
  diretos/indexados e padding com validacao contra streams truncados.
- [x] Primeiro schema grafico seguro Scene/Joint/DObj/PObj, mantendo o grafo em
  offsets de 32 bits validados.
- [x] Traversal de todos os PObjs desenhaveis de uma cena HSD, incluindo listas
  de display independentes e descritores de vertices por objeto.
- [x] Primeiro PObj real executado: `GmPause.dat` gera 48 triangulos no backend
  headless sem erros de display list.
- [x] Malha headless preserva position, normal, cor e UV por vertice de cada
  triangulo, inclusive quando os atributos chegam apos a posicao no stream GX.
- [x] Transformacoes SRT da arvore JObj sao acumuladas e aplicadas a cada PObj
  decodificado no backend headless.
- [x] Arrays GX provenientes de HSD carregam limites do segmento de dados;
  indices e strides que excedem o intervalo sao rejeitados sem leitura.
- [x] Streams GX NBT/NBT3 preservam normal, tangente e binormal por vertice
  no backend headless, para atributos diretos e indexados.
- [x] Transformacoes afins aplicam inverse-transpose normalizado a normais e
  transformacao direcional normalizada a tangentes/binormais.
- [x] JObjs com matriz independente do pai respeitam `JOBJ_MTX_INDEP_PARENT`
  durante o traversal headless.
- [x] Preview grafico SDL3/OpenGL para PObjs HSD: janela redimensionavel,
  camera orbitavel, profundidade, UVs, textura checker de fallback e cores por
  vertice via `melee-pc --view-pobj FILE SYMBOL`.
- [x] Schema MObj inicial: modo de renderizacao, TObj presente e material
  difuso/alpha HSD modulam os vertices apresentados pelo backend.
- [x] Decodificadores seguros de imagens GX `I4`, `I8`, `IA4`, `IA8`, `RGB565`,
  `RGB5A3`, `RGBA8` e `CMPR`, incluindo tileamento GameCube, validados contra as texturas
  referenciadas por `GmPause.dat`.
- [x] Cache de texturas por offset HSD e associacao por PObj: o preview SDL/OpenGL
  faz upload das imagens suportadas e usa a textura correta em cada triangulo.
- [x] Primeiro mapeamento de estado GX: `RENDER_XLU` por PObj controla blend
  alpha e escrita no depth buffer; o caso TEV basico usa textura × cor de vertice.
- [x] Renderer executa passes separados de opacos e translucidos para preservar
  depth dos objetos opacos antes do blend alpha.
- [x] Primeiro subconjunto de materiais GX concluido para os formatos presentes
  em `GmPause.dat` (I4/IA4), do HSD ate o backend SDL/OpenGL.
- [x] Texturas paletizadas GX `C4`, `C8` e `C14X2`: decodificacao TLUT
  (`IA8`, `RGB565`, `RGB5A3`), schema HSD de `HSD_TlutDesc` e associacao da
  paleta ao preview SDL/OpenGL, com limites validados.
- [x] Entrada SDL no preview: teclado ou primeiro SDL Gamepad alimenta o pad 0
  do snapshot host a cada tick, consumivel pelo `PADRead` original; hot-plug
  troca com seguranca entre gamepad e teclado.
- [x] Backend SDL/SDL_GameController para alimentar a entrada do host.
- [x] Adaptador puro de snapshot GameCube para os bits de navegacao usados por
  `Menu_GetAllInputs`, com botoes, analogo e gatilhos cobertos por teste.
- [x] Testes sinteticos de disco e HSD.
- [x] Primeiros modulos originais compilados nativamente: RNG, tempo, vetores,
  controlador, memoria, objalloc/list e fila `devcom` da baselib.
- [x] Nucleo vertical de luta compilado nativamente: configuracao de partida
  (`gmmain.c`/`gmmain_lib.c`), jogadores (`player.c`), lutadores
  (`fighter.c`) e terreno (`ground.c`). Ainda nao e ligado ao executavel.
- [x] Rota de menu e VS compilada nativamente: `mnmain.c`, `gmscene.c`,
  `gmvsmode.c`, `gmvsmelee.c` e `gmvs.c`.
- [x] Fachada C ABI para o armazenamento original de regras VS: leitura,
  escrita e restauração dos defaults de `gmMainLib_DefaultGameRules`, testadas
  sem expor structs PPC ao C++ host.
- [x] Preparação de `StartMeleeData` original para uma luta VS local de dois
  jogadores, com regras atuais, personagens e estágio validados por uma ABI
  host segura. A transição para a cena ainda depende das facades de runtime.
- [x] Inicialização completa dos seis slots originais de jogador: estado base,
  tabela de stale moves, estatísticas de ataque e estado de bônus; os dois
  jogadores preparados são materializados nos slots antes da criação de
  objetos Fighter.
- [x] Caminho de input de menu executado: snapshot host → HSD Pad →
  `gm_EvaluateAllControllerInputs` original → mapeamento de eventos compativel
  com `mn_80229624`, com A/confirmacao cobertos por teste integrado.
- [x] Compatibilidade host de 64 bits para asserts de layout PPC, diagnostico
  `OSPanic`, tempo e declaracoes Dolphin compartilhadas, sem alterar o caminho
  matching.
- [x] Sistema de classes HSD original (`class.c`, `object.c`, `hash.c`)
  compilado nativamente, com o alocador de blocos por tamanho operando sobre o
  heap host de 64 bits.
- [x] Runtime de objetos de cena HSD compilado nativamente: `gobj.c`,
  `gobjinit.c`, `gobjproc.c`, `gobjplink.c`, `gobjgxlink.c`, `gobjobject.c` e
  `gobjuserdata.c`. As quatro classes embutidas (camera, luz, joint e fog) sao
  registradas na ordem original.
- [x] Escalonador de frame original executando no host: `HSD_GObj_RunProcs`
  percorre os processos por prioridade, respeita a mascara de p_links pausados
  apontada por `HSD_GObjLibInitData.unk_2` e aplica a remocao adiada quando um
  processo libera o proprio GObj.
- [x] Fachada C ABI `melee_host_scene_runtime_*` com handles opacos de 32 bits
  para objetos de cena, sem expor structs de layout PPC ao host.
- [x] Diagnostico `melee-pc --diagnose-scene-runtime [FRAMES]` executando o
  escalonador original por N frames.
- [x] Conjunto paired-single portavel completo para a camada grafica:
  `PSMTXInverse`, `PSMTXInvXpose`, `PSMTXTranspose`, `PSMTXMultVec`,
  `PSMTXMultVecSR`, `PSMTXMultVecArray`, `PSMTXRotAxisRad` e `PSVECAdd`, todos
  seguros para destino aliasado como o codigo original exige.
- [x] `spline.c` original compilado nativamente, substituindo a facade
  artesanal `hsd_spline.c`. `splGetSplinePoint` e `splArcLengthPoint` passam a
  vir do codigo decompilado.
- [x] Matrizes de projecao e vista portaveis: `MTXFrustum`, `MTXPerspective`,
  `MTXOrtho`, `C_MTXLookAt` e `MTXRotRad`.
- [x] Subset de estado GX host em `port/src/gx/state_recorder.cpp`: o host
  modela o estado que a API GX descreve, nao os registradores do Flipper.
  Cobre profundidade, blend, alpha compare, culling, scissor, viewport,
  projecao, memoria de matrizes, estagios TEV completos (entradas, operacoes,
  konstantes, swap e registradores S10), texgen, canais de iluminacao com
  cores de ambiente e material, objetos de textura e TLUT, luzes com
  atenuacao angular e por distancia, fog e copias de EFB.
- [x] Os objetos opacos da SDK (`GXTexObj`, `GXTlutObj`, `GXLightObj`) guardam
  seu conteudo dentro do proprio blob, com ponteiros de 64 bits divididos em
  dois campos de 32 bits em vez de truncados, e movidos por copia explicita
  para nao depender de type punning.
- [x] Copia de display e sincronizacao de desenho GX: `GXSetDispCopySrc/Dst`,
  `GXSetDispCopyYScale` com contagem de linhas, gamma, clamp, cor de limpeza,
  filtro de copia com padrao de amostragem e pesos, `GXCopyDisp`,
  `GXSetDrawDone`, `GXWaitDrawDone`, `GXDrawDone` e `GXSetDrawDoneCallback`.
  O host nao tem processador grafico assincrono, entao a fence de draw-done
  fica pendente ate alguem drena-la, e `melee_host_gx_drain_draw_done` permite
  que o laco de frame entregue o callback em um ponto deterministico.
- [x] `GXNtsc480IntDf`, o render mode NTSC 480i com deflicker que `gmMain`
  instala, com os valores da SDK.
- [x] Camada VI do host em `port/src/video/vi.cpp`: contador de retrace,
  paridade de campo, callbacks pre e pos retrace na ordem original e latch de
  registradores sombra no retrace seguinte ao `VIFlush`. Nada dorme nem le
  relogio de parede: o tempo avanca quando o jogo bloqueia em
  `VIWaitForRetrace` ou quando o laco do host chama
  `melee_host_video_advance_retrace`.
- [x] Presets de debug/sanitizers e workflow multiplataforma.

## Em andamento

- [ ] Compilar todo o codigo relevante sem assembly PPC.
- [ ] Resource manager runtime consumindo o manifesto extraido.
- [ ] Cancelamento, streaming e prioridade completa da API DVD.
- [ ] Loader HSD com schemas Disk/Runtime e referencias ciclicas.
- [ ] Fluxo vertical de luta local: `StartMeleeData` → cena VS → players →
  loop de frame (roteiro em `docs/fight_flow_port.md`). O escalonador de frame
  ja roda; falta a camada de objetos graficos que alimenta os callbacks de
  render.
- [ ] Camada de objetos graficos HSD. Enquanto nao existe,
  `port/src/game/hsd_graphics_stubs.c` fornece substitutos que abortam com
  diagnostico em vez de retornar valores neutros.

  Os onze arquivos `cobj.c`, `lobj.c`, `jobj.c`, `dobj.c`, `mobj.c`, `tobj.c`,
  `pobj.c`, `robj.c`, `wobj.c`, `fog.c` e `displayfunc.c` compilam sem erro em
  x86-64, mas formam um unico bloco: cada um referencia os outros, entao
  adicionar qualquer um exige adicionar todos.

  O gap caiu de 73 para 33 simbolos com o subset de estado GX e a matematica
  de matrizes. Nenhum simbolo da API GX bruta falta mais. Os 33 restantes sao
  todos modulos HSD ainda nao compilados: `texp.c` (12), `state.c` (7),
  `tev.c` (4), `util.c` (2), `perf.c` (2), `bytecode.c` (1),
  `initialize.c` (1), `video.c` (1) e `VIGetNextField` da SDK.

  A segunda onda, que aparece ao incluir esses modulos, comecou com 56
  simbolos e esta em 14. O GX e o VI foram fechados. Restam:

  - API de heap do OS (10): `OSInitAlloc`, `OSCreateHeap`, `OSAllocFromHeap`,
    `OSSetCurrentHeap`, `OSDestroyHeap`, `__OSCurrHeap`, `OSGetArenaHi`,
    `OSGetArenaLo`, `OSSetArenaLo`, `OSGetPhysicalMemSize`.
  - Quatro simbolos HSD: `HSD_LogInit` (`debug.c`), `HSD_ShadowGetAllocData` e
    `HSD_ShadowInitAllocData` (`shadow.c`), `HSD_Synth_804D6018` (`synth.c`).

  O heap do OS e o proximo recorte, e e mais delicado que os anteriores.
  `OSAlloc.c` e `OSArena.c` compilam nativamente sem simbolos pendentes alem
  de `OSReport`, mas nao sao seguros em 64 bits: ha 25 pontos em `OSAlloc.c`
  que convertem ponteiro para `u32`, incluindo aritmetica carregada como
  `cell = (void*)((u32)ptr - 0x20)` e mascaras literais `& 0xFFFFFFE0` que
  zerariam a parte alta de um endereco. As macros `ROUND`, `TRUNC` e `OFFSET`
  de `extern/dolphin/include/dolphin/os.h` tem o mesmo problema e sao usadas
  por `OSArena.c`. A correcao segue o padrao ja estabelecido em
  `src/sysdolphin/baselib/objalloc.c`: um typedef de endereco sob
  `MELEE_HOST` que vira `uintptr_t` no host e continua `u32` no caminho
  matching, deixando o DOL inalterado.

## Proximos gates

1. Inicializar o primeiro grafo de audio sem DSP/ARAM fisico.
2. Compilar o primeiro fluxo de menu sobre as facades host.
3. Carregar uma cena real que use TLUT no preview e cobrir seu asset em teste
   de integracao.

## Limitacoes atuais

- Os assets `GALE01` extraidos estao disponiveis apenas em `assets-local`, que
  permanece ignorado pelo Git e nao faz parte de builds ou artefatos publicos.
- O executavel ainda nao chama `gmMain`.
- GX (alem do recorder/VCD/VAT), AX, CARD, streaming DVD e THP ainda nao estao
  implementados. PAD e DVD assincrono tem pontes basicas; os backends completos
  ainda faltam.
- O runtime GObj executa processos, mas nenhum GObj pode receber um objeto
  grafico ainda: anexar camera, luz, joint ou fog leva aos substitutos de
  `hsd_graphics_stubs.c`, que abortam de proposito.
- `GXInitFogAdjTable` grava a tabela neutra (256, ou 1.0 em ponto fixo 8.8).
  A derivacao real a partir da projecao nao esta modelada, e o estado de fog
  reporta isso em `range_adjust_modelled`.
- `GXCopyTex` e `GXCopyDisp` registram o pedido de copia mas nao produzem
  pixels: nao ha framebuffer host ainda.
- Na SDK o callback de draw-done tambem pode chegar por interrupcao, sem
  espera. O host nao reproduz isso: sem processador grafico assincrono, a
  fence so e entregue por `GXWaitDrawDone`, `GXDrawDone` ou pelo laco de
  frame. Codigo que dependa da entrega por interrupcao nao veria o callback.
- `VIGetDTVStatus` reporta ausencia de saida digital em vez de adivinhar um
  modo progressivo que o host nao honraria.
- `hsd_3A76.c` chama `MTXOrtho` com um buffer `Mtx` de tres linhas onde uma
  projecao precisa de quatro. No caminho PowerPC isso e coberto pelo bloco
  `MUST_MATCH`; no host seria estouro de pilha. O arquivo ainda nao entra no
  build host, mas precisa de atencao quando entrar.
- A build matching nao foi executada porque `orig/GALE01/sys/main.dol` nao esta
  presente; as mudancas compartilhadas estao isoladas por `MELEE_HOST`.
