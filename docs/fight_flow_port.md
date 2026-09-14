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
- Executado: o modo `GM_VS` até a luta. A tabela do host tem a entrada do modo
  e as cenas de seleção de personagens (`GS_CSS`, `mncharsel.c`) e de estágio
  (`GS_SSS`, `mnstagesel.c`). Com dois pads roteirizados, as duas portas abrem
  como HMN, os dois jogadores escolhem Fox, START leva à SSS e o cursor escolhe
  Hyrule Temple; o `VsModeData` fica com estágio 14 e Fox nos slots 0 e 1
  (teste `melee-host-vs-selection-asset`). O modo VS do host termina na luta:
  resultados, morte súbita e desafiante ficam fora (`gmvsmode.c` sob
  `MELEE_HOST`).
- Próximo bloqueio: a cena de luta (`GS_VS`). A tabela do host precisa de
  `gm_Scene_Vs_OnFrame`, `gm_Scene_Vs_OnEnter` e `gm_Scene_Vs_OnExit` (`gmvs.c`),
  e o estado VS roda antes `gmVsMelee_EnterVs`, que monta o `StartMeleeData`.
  `gm_Scene_Vs_OnEnter` passa por `fn_8016E730`, pelo HUD (`ifStatus`,
  `ifTime`) e dali aos lutadores e ao estágio. O alvo é Fox vs. Fox em Hyrule
  Temple (`grshrine.c`, o menor módulo de estágio liberado sem cartão; Final
  Destination e Battlefield estão travados).
- Medido em 13/09/2026: só as chamadas diretas de `fn_8016E730` e
  `gm_Scene_Vs_OnEnter` caem em 15 módulos fora do build: `cm/camera.c`,
  `ef/eflib.c`, `mp/mpcoll.c`, `it/item.c`, `it/itspawn.c`, `if/ifall.c`,
  `if/if_2F6E.c`, `if/iftime.c`, `if/ifstatus.c`, `gm/gmpause.c`,
  `ft/ftdevice.c`, `lb/lbrefract.c`, `lb/lb_00F9.c`, `lb/lb_0219.c` e
  `sfx/sfx_unk.c`. `efAsync_LoadSync(0)` e `(0x1F)` carregam bancos de
  partícula, que param em `psInitDataBankLocate` (relocação de 32 bits no
  lugar), então as partículas entram cedo nesse recorte.
- Primeira onda de link, medida em 13/09/2026 com `GS_VS` posto na tabela do
  host: 437 símbolos indefinidos, todos no `melee-pc` (o binário de testes não
  liga a tabela de cenas). Quem referencia: `fighter.c` (218), `gmvs.c` (58),
  `ftkirby.c` (47), `gm_1601.c` (30), `dbinit.c` (21), `ground.c` (19),
  `plbonus.c` (16) e o resto em arquivos de lutador. Onde estão definidos,
  em cerca de 75 arquivos: o núcleo de lutador (`ftcommon.c`, `ftcoll.c`,
  `ftparts.c`, `ftdynamics.c`, `ftanim.c`, `ftlib.c`, `ft_08*.c`, `ftCo_*`),
  `plbonuslib.c` (42), `camera.c` (16), itens, HUD, colisão (`mplib.c`,
  `mpcoll.c`), `particle.c` (`psInitDataBank`, `psInitDataBankLoad`) e
  `it_804D6D38`, que não tem definição em C. Casos que pedem decisão antes de
  compilar: os 19 handlers do menu de depuração (`db*.c`), que `dbinit.c` só
  alcança em níveis de depuração; `grpstadium.c`, que `ground.c` referencia
  direto; e `gm_16A2.c`, `gm_17C0.c`, `gm_17EB.c` e `gmregclear.c`, que `gmvs.c`
  referencia para outros modos.
- Como abrir as ondas sem quebrar o que já roda: meça com a entrada `GS_VS` só
  localmente e commite ondas que compilam sem ela. Desde que o `host-sanitize`
  descarta no link o que nada chama (`-fsanitize-address-globals-dead-stripping`
  e `-Wl,-z,start-stop-gc`), um módulo compilado e não chamado não muda o link
  de nenhum dos dois presets, então dá para compilar o resto da decomp antes de
  ligá-lo à cena. A sondagem de 13/09/2026 mostra que 741 dos 808 arquivos fora
  do core já passam em `-fsyntax-only`.
- Plano a partir daí: (1) corrigir as 67 falhas de compilação e pôr o resto de
  `src/melee` no core, menos `gmscdata.c`, retirando de `unported.c` as paradas
  que os módulos novos passam a definir; (2) com `GS_VS` na tabela, o que
  continuar indefinido é SDK, MSL ou asm, a tratar caso a caso; (3) rodar a
  rota até a luta e seguir os crashes.
- Executado o passo (1): os 808 arquivos compilam e estão no core, com
  `particle.c`, `generator.c`, `psappsrt.c`, `quatlib.c` e `src/MSL/float.c`.
  As rotas existentes não mudaram. Detalhes em `docs/native_port_status.md`.
- Executado o passo (2): com `psdisp.c`, `psdisptev.c`, textura indireta e
  `GXEnableTexOffsets` no host, a cena de luta liga sem símbolo indefinido.
- Em andamento o passo (3). Com `GS_VS` na tabela (só local), a rota entra na
  luta. `lbRefData`, o primeiro dado que ela pede, já tem tradutor.
- Executado o loader de partículas (detalhes em `docs/native_port_status.md`).
  A API de arquivo do host traduz `eff*DataTable` e `map_ptcl`/`map_texg` em
  bancos já localizados, em largura de ponteiro, e `psInitDataBankLocate` e
  `psInitDataBankLoad` tomam as tabelas deles. Os 36 arquivos de efeito e os
  20 pares de banco de estágio do disco traduzem. Com `GS_VS` na tabela (só
  local), a entrada da luta passa por `efAsync_LoadSync(0)` e `(0x1F)` e para
  em `Player_80036DD8`, que pede `plLoadCommonData` a `PdPm.dat`.
- Executado o tradutor de `plLoadCommonData`. A entrada passa pelos dados de
  jogador e pelo começo do estágio (`ftCo_800C06C0`, `mpColl_80041C78`,
  `Ground_801C0378`) e cai em `Ground_801C0754`, porque a entrada de Hyrule
  Temple em `stage_datas` é nula: o host liga os estágios por referência fraca
  (`port/src/game/host_weak_stages.h`), o que não puxa `grshrine.c` da
  biblioteca estática.
- Executado: `stage_datas` liga forte. Sem `host_weak_stages.h`, o `melee-pc`
  liga todos os estágios da tabela sem símbolo indefinido, `Ground_801C0754`
  acha Hyrule Temple e `grDatFiles_801C6038` lê `GrSh.dat`. A entrada para nos
  dados do estágio: nenhum dos oito símbolos que ela pede tem tradução
  (`map_head`, `coll_data`, `grGroundParam`, `itemdata`, `ALDYakuAll`,
  `yakumono_param`, `map_plit`, `quake_model_set`), e `Ground_801C28CC` segue
  o `stage_info.param` nulo.
- Próximo bloqueio: esses oito tradutores, presentes nos 71 `Gr*.dat`
  (`map_plit` e `quake_model_set` em 67). `map_head` (`UnkStageDat`) é o
  maior: modelos com câmera, luzes e fog (`UnkStageDat_x8_t`), splines,
  sombras e marcadores de animação. `coll_data` (`MapCollData`) traz vértices,
  linhas e juntas de colisão; `grGroundParam` (`GroundParam`) mistura
  escalares, cores e um vetor de `StageParam`.
- Executado o tradutor de `grGroundParam` (os 71 `Gr*.dat` traduzem). A
  entrada passa por `Ground_801C28CC` e, ainda em `Ground_801C0754`, chega a
  `Ground_801C5878`, que inicia o display de troféus (`tyDisplay_8031C2CC`):
  `Toy_803124BC` pede a `TyDatai.usd` sete tabelas (`tyInitModelTbl`,
  `tyInitModelDTbl`, `tyModelSortTbl`, `tyExpDifferentTbl`, `tyNoGetUsTbl`,
  `tyDisplayModelTbl`, `tyDisplayModelUsTbl`), todas vetores de structs só de
  escalares terminados por -1, e para na primeira. A falta de `map_head` não é
  fatal até ali.
- Executados os tradutores das sete tabelas de troféu. Com elas a entrada do
  estágio termina (`Stage_802251E8` retorna) e `fn_8016E730` segue até
  `Item_80266F70`, onde `it_8027870C` pede `itPublicData` a `ItCo.usd`; o
  registro guarda em `x0`–`x14` as seis tabelas comuns de item que ela copia
  para `it_804D6D28`, `it_804D6D24`, `it_804D6D38`, `it_804D6D30`,
  `it_804D6D40` e `it_804D6D04`. É o próximo tradutor. Os dados do estágio que
  faltam (`map_head`, `coll_data` e mais cinco) ainda não pararam a entrada.
- Levantado para o tradutor de `itPublicData` (medido em `ItCo.usd` em
  14/09/2026, com um script sobre as relocações do arquivo):
  - O registro tem seis ponteiros: `ItemCommonData` (`data+0x2FC`), as tabelas
    de `Article*` dos 43 itens comuns (`0x3EAC`), dos 118 itens de personagem
    (`0x4EC8`) e dos 47 Pokémon (`0xBB38`), `it_804D6D40_t` e
    `Fighter_804D653C_t`. As contagens vêm de `It_Kind_Kuriboh` (43),
    `It_PKind_Start` (161) e `It_Kind_Old_Kuri` (208), e a tabela de Pokémon
    termina exatamente onde `it_804D6D40_t` começa. Os itens de estágio
    (`it_804A0F60`) não vêm de `ItCo`.
  - Os 43 itens comuns e os 47 Pokémon estão todos presentes; dos 118 de
    personagem, só 8 ponteiros não são nulos no arquivo. Cada `Article` aponta
    atributos comuns (`ItemAttr`, com bit-fields e `itECB`), atributos
    próprios em `void*`, hurtboxes (`ItHurtBoneList`: 14 comuns, 6 de
    personagem, 4 Pokémon), estados (`ItemStateDesc`: três animações e o
    script), modelo (`ItemModelDesc`) e dinâmica (`BoneDynamicsDesc`, só em 3
    itens comuns).
  - Os atributos próprios mudam de layout por item. Em 11 blocos há ponteiros
    relocados (itens comuns 18, 26, 27 e 29; de personagem 0 a 3 e 115;
    Pokémon 11 e 38), então eles pedem tradução por tipo; os outros não têm
    ponteiro, mas só a struct de cada item diz a largura dos campos.
  - O script de cada estado (`xC_script`) é lido por `itanimlist.c` pela
    `CmdUnion` de `lb/types.h`, que declara cada comando como bit-fields sobre
    `u32` (`opcode : 6` primeiro). O MWCC aloca bit-fields a partir do bit mais
    significativo e o GCC do host, em little-endian, a partir do menos
    significativo, então o fluxo não pode ficar verbatim como as animações. É o
    formato dos scripts de lutador também: os leitores são `ftaction.c`,
    `ftcolanim.c`, `grmaterial.c`, `itanimlist.c`, `lbcommand.c`, `lb_013B.c`
    e `lb_0219.c`, e as structs de comando têm 295 linhas de bit-field. A
    decisão vale para itens e lutadores e deve vir antes do tradutor. Medido
    em 14/09/2026: com `__attribute__((scalar_storage_order("big-endian")))`
    numa cópia de `struct unk0`, o GCC 16 lê `opcode`, `unk1` e `unk2` certos
    de bytes big-endian, mas o `host-sanitize` compila com clang 22, que ignora
    o atributo com um aviso. O caminho que serve aos dois presets é converter
    as palavras do fluxo e declarar os campos em ordem inversa sob
    `MELEE_HOST`, ou ler os campos por acessores.
- Executada a decisão dos scripts de comando (detalhes em
  `docs/native_port_status.md`): a API de arquivo converte as palavras de um
  script para a ordem nativa no lugar, as structs de comando do host vêm de
  `port/tools/gen_host_command_layout.py` com os campos na ordem inversa, as
  leituras por cast passam por `CMD_U8`/`CMD_U16`/`CMD_S16`, e os alvos de
  sub-rotina e goto viram distâncias relativas. Vale para itens, lutadores e
  sobreposição de cor. Nos 150 scripts de estado de `ItCo.usd` a regra de
  parada vale: 154 trechos convertidos, nenhum ponteiro fora de sub-rotina ou
  goto, e todos terminam em reset, return ou goto antes da fronteira. O
  próximo passo é o tradutor de `itPublicData`.
- Executado o tradutor de `itPublicData` (detalhes em
  `docs/native_port_status.md`), com as restrições RObj de bytecode que os
  modelos de item usam. Atributos próprios de cada tipo de item e dinâmica
  ficam de fora e param com nome quando um item que os tenha é criado. Com
  `GS_VS` na tabela (só local), a entrada passa pelos itens
  (`Item_80266F70`, `Item_80266FCC`, `it_8026D018`) e pelo áudio e cai em
  `Ground_801C1E94` (chamado por `Ground_801C0800`), que lê o `map_head` do
  estágio (`UnkStageDat`, via `grDatFiles_GetArchive()->unk4`), ainda sem
  tradução.
- Próximo bloqueio: os dados de estágio, começando por `map_head`. Ele aponta
  os modelos com câmera, luzes e fog (`UnkStageDat_x8_t`), splines, sombras e
  a tabela de luzes que `LightOverrideEntry` compara por ponteiro com as
  `LightList` dos modelos; por isso o tradutor precisa entregar o mesmo
  `HSD_LightDesc*` nas duas estruturas.
- Como era o bloqueio dos efeitos: `efAsync_LoadSync(0)` carrega `EfCoData.dat`
  e pede `effCommonDataTable`, cuja estrutura aponta os bancos de comando e de
  textura das partículas (e os modelos dos efeitos). No console
  `psInitDataBankLocate` reloca os bancos no lugar com endereços de 32 bits e
  `psInitDataBankLoad` monta tabelas de ponteiros para dentro deles
  (`psCmdListArray`, `ptclref_804D0E5C`, `psTexGroupArray`,
  `psNumCmdList`). O host precisa de um loader de partículas que construa
  essas tabelas em largura de ponteiro, mantendo os streams de comando
  verbatim, como a animação e as display lists, e traduza os grupos de
  textura. Os três pontos a portar juntos são a tabela de efeitos, os bancos
  e o interpretador de comandos de `particle.c`, que lê esses streams.
- O que já foi levantado para esse loader:
  - A tabela (`effCommonDataTable` em `data+0` de `EfCoData.dat`) começa com
    dois ponteiros, banco de comandos e banco de texturas, que `efAsync_OnLoad`
    passa a `psInitDataBankLocate`. A partir de `+0x8` vem um vetor de
    `EF_EffectDesc` (`f32` de duração e um `StaticModelDesc`, 0x14 bytes no
    console), que `efLib_Create` indexa por `gfx_id % 1000` sem limite; o
    tamanho do vetor não está gravado e precisa ser deduzido, por exemplo pelo
    primeiro alvo de relocação depois da tabela.
  - `HSD_PSCmdList` não tem ponteiros: um cabeçalho de 0x3C bytes (u16, u32 e
    floats) seguido dos bytes de comando embutidos. No host o cabeçalho pode
    ser convertido e os bytes copiados como estão; o fim de cada lista vem do
    início da seguinte ou do fim do banco.
  - `HSD_PSTexGroup` tem escalares (`num`, `fmt`, `tlutfmt`, `width`,
    `height`, `palnum`, `palflag`) e um vetor de ponteiros para imagens e
    paletas em `+0x18`, que no host dobra de largura. Os texels e as paletas
    ficam verbatim, porque os decodificadores GX já os leem big-endian.
  - `psInitDataBankLoad` guarda a contagem de texturas com
    `((s32*) psFormGroupArray)[bank]`, o que no host escreve na metade de um
    ponteiro; o caminho do host precisa de onde guardar essa contagem.
  - O interpretador monta operandos de 16 bits byte a byte, em big-endian, mas
    `psReadFloat` copia os quatro bytes para `hsd_804D78D0` e o lê como `f32`,
    o que no host inverte a ordem: precisa de correção sob `MELEE_HOST`.
  - Os efeitos passam `formBank` e `ref` nulos; as formas (`formTable`) e as
    leituras `*(u32*)`/`*(f32*)` delas em `psdisp.c` só importam quando um
    banco de formas existir.
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
