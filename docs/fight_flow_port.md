# Fluxo de luta nativo

## Meta vertical

Chegar a uma partida local com dois jogadores, a partir de `StartMeleeData`,
sem emulação do GameCube:

1. menu produz `StartMeleeData`;
2. scene manager seleciona uma cena VS e o estágio;
3. `Player_InitAllPlayers` materializa os jogadores;
4. `fighter.c` e `ground.c` executam o loop por frame;
5. PAD, DVD/HSD, renderer e áudio host atendem o fluxo.

## Primeiro ponto de entrada

`src/melee/pl/player.c:Player_InitAllPlayers` é o primeiro entrypoint de
runtime dos jogadores. As cenas de demonstração em `src/melee/vi/` mostram a
sequência mínima: inicialização de objetos de lutador, `Player_InitAllPlayers`
e entrada de cena. O fluxo VS deve reutilizar essa sequência, mas receber
`StartMeleeData` produzido por menu.

## Gates de compilação

- Compilado: controlador de menu (`mnmain.c`), scene manager (`gmscene.c`) e
  rota VS (`gmvsmode.c`, `gmvsmelee.c`, `gmvs.c`). Ainda falta conectá-los às
  facades do host e executar a transição.
- Executado: a entrada de menu percorre o HSD Pad e o avaliador original; a
  saída usa um mapeamento host compativel com `mn_80229624`, sem carregar o
  grafo visual ainda pendente. A configuração VS usa o armazenamento original
  de `gmMainLib` por uma ABI host sem layouts PPC expostos.
- Compilado: núcleo de partida (`gmmain.c`/`gmmain_lib.c`), `player.c`,
  `fighter.c` e `ground.c` sem assembly PPC. O próximo recorte deve resolver
  suas facades e ligá-los a um entrypoint host, não apenas ao arquivo estático.
- Executado: o escalonador de frame original. `HSD_GObjInit` sobe com os tetos
  de prioridade que `gmScene_Init` usa (`gproc_pri_max` 0x18) e
  `HSD_GObj_RunProcs` roda por frame no host. É esse laço que `gmscene.c`,
  `player.c`, `ground.c` e `fighter.c` esperam para executar por frame.
- Executado: a camada de objetos gráficos HSD. `ground.c` e `fighter.c` criam
  GObjs e anexam JObj/CObj/LObj, e esses caminhos agora chegam ao código real
  em vez dos substitutos que abortavam. Cenas e modelos do disco entram por
  `HSD_JObjLoadJoint` sobre descritores materializados em layout host; ver
  `docs/native_port_status.md`.
- Executado: o caminho de render original. `HSD_JObjDispAll` percorre a árvore
  carregada e as display lists do asset chegam ao recorder GX do host, que é o
  mesmo caminho que `ftdrawcommon.c` e `itdraw.c` tomam por frame.
- Executado: a imagem do caminho original. O recorder captura texgen,
  conjuntos de textura e os dois canais rasterizados, e o preview avalia o
  programa TEV de cada draw por fragmento num shader gerado com a mesma
  semântica da referência de CPU (`port/src/gx/tev.cpp`), conferido pixel a
  pixel por `--tev-conformance-*`. Ainda faltam fog, bump e cópias de EFB.
- Executado: a API de arquivo HSD pela qual o jogo pede assets.
  `HSD_ArchiveParse` e `HSD_ArchiveGetPublicAddress` são do host e
  reconstroem cada símbolo em layout de 64 bits pelo sufixo do nome; a lista
  que `gmTitle_801A1AC0` passa a `lbArchive_LoadSymbols` traduz inteira e
  carrega pelos loaders originais. Detalhes em `docs/native_port_status.md`.
- Executado: a memória de boot e o carregador do jogo. A sequência de `gmMain`
  (`HSD_InitComponent`, `lbMemory_8001564C`, `lbHeap_80015F3C`) e o fim do
  setup de heap de cena (`lbHeap_80015900`) rodam no host, e
  `lbArchive_LoadSymbols` lê o arquivo da tela de título por `lbFile`, pela
  fila devcom e pelo DVD do host.
- Executado: a primeira cena do jogo entrando pelo próprio código. O boot
  segue o `main()` original, e `gm_801A4BD4` e `gm_Scene_Title_OnEnter` montam
  câmeras, luz, fog e os dois modelos do título.
- Executado: o laço de frame original (`gm_801A4D34`) na tela de título, com o
  relógio do OS congelado: 621 frames até a cena sair sozinha, todos
  desenhados por `HSD_GObj_80390FC0` e copiados para XFB pelo ciclo de
  `video.c`.
- Executado: a tela de título apresentada pelo SDL/OpenGL com a câmera do
  jogo (`melee-pc --view-title-scene`): cada frame do laço original desenhado
  com a projeção, o viewport e o scissor de cada draw, a 60 Hz, com teclado ou
  gamepad como PAD.
- Executado: o modo de título inteiro por `runGameMode`
  (`melee-pc --run-modes`), pela tabela de modos e cenas do host: o
  preload do estado, que mantém todos os heaps de preload, a demo do título
  carregando lutadores, estágio e efeitos em segundo plano (inclusive para a
  ARAM), o laço de frame e o `onExit` original. Sem botões o modo seguinte é o
  filme de abertura; START leva ao menu (`GM_MENU`).
- Executado: o menu principal (`GM_MENU`), do título ao VS Melee pelo roteamento
  do gerenciador de cenas, com botões apertados por frame: o menu entra, carrega
  seus modelos, textos SIS, a tabela de eventos e os dados de áudio, desenha o
  texto pelo interpretador original e sai para `GM_VS` com DOWN, A e A. A música
  não começa, porque o host ainda não entrega vozes.
- Próximo bloqueio: o modo `GM_VS`. A tabela do host precisa da entrada do modo
  (`gmVsMelee_Mode_OnLoad`, `gm_Mode_Vs_OnUnload` e `gmVsMelee_Mode_OnInit`,
  sem preload de modo) e das cenas dos seus estados, que já compilam em
  `gmvsmode.c`: primeiro a seleção de personagens (`GS_CSS`,
  `mnCharSel_Scene_*` em `mncharsel.c`), depois a de estágio (`GS_SSS`,
  `mnStageSel_Scene_*` em `mnstagesel.c`) e então a luta (`GS_VS`). Os dois
  arquivos de seleção ainda não entram no build.
- A matemática paired-single, o subset de estado GX e a camada VI que essa
  camada consome já estão prontos e testados. O laço de frame já tem as duas
  metades que precisava: `HSD_GObj_RunProcs` para a simulação e o retrace de
  VI para a apresentação, ambos determinísticos e sem dormir.
- O heap do OS já roda: o alocador original entrega blocos em endereços de 64
  bits reais, e `initialize.c` entrou junto com a camada de objetos gráficos.
  Detalhes em `docs/native_port_status.md`.
- Implementar as facades GX, áudio e DVD que surgirem durante esse recorte,
  apenas conforme necessárias.

## Critério de aceite

Dois controles host selecionam lutadores, entram em uma cena de estágio, os
lutadores recebem input em ticks determinísticos e a cena é apresentada pelo
backend SDL/OpenGL.
