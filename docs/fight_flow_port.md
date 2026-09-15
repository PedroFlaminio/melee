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
- Executado o tradutor de `map_head` (detalhes em
  `docs/native_port_status.md`); 69 dos 71 estágios traduzem. Com `GS_VS` na
  tabela (só local), a entrada passa por `Stage_8022524C` inteiro (o jogo
  avisa "use dummy CamRange" e "use dummy DeadRange", porque o resto dos dados
  de estágio ainda não chega), pela câmera e por `fn_8016E2BC`, e para em
  `Fighter_LoadCommonData`, que pede `ftLoadCommonData` a `PlCo.dat`.
- Próximo bloqueio: `ftLoadCommonData`, um registro de 23 ponteiros que
  `Fighter_LoadCommonData` copia para globais. Levantado em 14/09/2026:
  - Só escalares, sem relocação dentro: `ftCommonData` (0x818 bytes; as
    exceções em bytes são `x6DC_colorsByPlayer`, `x6EC` e `x7D8`), as linhas
    de `ftCo_ItemThrowAttrs` de `Fighter_804D6550` (que `ftCo_ItemThrow.c`
    percorre por aritmética com offsets do console, válida porque as linhas
    são só floats), `654C` (linhas de 5 floats), `6548`, os modificadores de
    escala, coelho, metal e gravidade (`6524` a `6518`), `CrowdConfig`, `6510`
    (sem leitor) e os bytes de `650C` e `6508`.
  - Com ponteiros: `ftPartsTable` (por tipo de lutador, dois vetores de `u8`
    e a contagem), `Fighter_804D6540` (por tipo, ponteiro e contagem de
    registros de `u8`), as tabelas de scripts de cor `653C` e `6538` (lidas
    por `lb_800144C8`, como as dos itens), `6534` (o joint e a animação do
    pedestal de reaparecimento), `6530` (pares de `Vec2*` e contagem, com a
    contagem lida de um slot de ponteiro), as duas tabelas de tremor, os
    joints de `6514` (pedestal de troféu) e `6504`, e `Fighter_804D64FC`, as
    tabelas da IA de CPU por personagem, com os scripts de `cmdscripts`.
- Executado o tradutor de `ftLoadCommonData` (detalhes em
  `docs/native_port_status.md`). Com `GS_VS` na tabela (só local), a entrada
  passa pelo resto de `Fighter_800679B0` e pela inicialização dos jogadores e
  para em `Fighter_Create` → `ftData_8008572C`, que pede `ftDataFox` a
  `PlFx.dat`: os dados próprios do personagem.
- Executado o tradutor de `ftDataFox` (detalhes em
  `docs/native_port_status.md`). Com `GS_VS` na tabela (só local), os dois Fox
  são criados: `fn_8016E2BC` termina, com dados, fantasia, animações e posição
  inicial. Para isso também foi preciso montar a fila de ARAM (`lbarq.c`) no
  boot e manter, no `map_head`, o joint de cada entrada de pares, pelo qual
  `Ground_801C34AC` registra os pontos de partida dos jogadores.
- Bloqueio seguinte, resolvido: `fn_8016E730` → `fn_801A1134` (`gmpause.c:86`)
  carrega `ScGamPause_scene_data` de `GmPause.dat`, o modelo do menu de pausa,
  um `_scene_data` (`SceneDesc`). A API de arquivo passou a atender o tipo;
  câmeras e fogs são vetores sem terminador, contados enquanto a entrada tem
  descritor e até a próxima fronteira.
- Bloqueio seguinte, resolvido: o HUD. `ifAll_802F390C` carrega
  `ScInfDmg_scene_data` e os modelos de `IfAll.dat`: os `_scene_models`
  (`ScInfCnt`, `DmgNum`, `DmgMrk`, `ScInfTim`, `ScInfPnm`, `ScInfStc`) e, sem
  sufixo, `Stc_scemdls`, `Stc_rarwmdls`, `tdsce` e `lupe`, todos tabelas de
  `DynamicModelDesc*` terminadas por NULL (`lupe` é uma tabela de um, que
  `ifmagnify.c` lê por `*(DynamicModelDesc**)`). Depois, `lbBgFlash_Init` pede
  `lbBgFlashColAnimData` a `LbBf.dat`, as animações de cor no formato das dos
  itens.
- Próximo bloqueio: `fn_8016E730` termina. Em `gm_Scene_Vs_OnEnter`,
  `ifStatus_802F665C` → `ifStatus_802F5EC0` (`ifstatus.c:712`) acha os dígitos
  de dano com `ifStatus_802F6194`, que recebe um JObj convertido em GObj e
  anda por `next_gx` e `next`: no console esses campos caem onde o JObj guarda
  `child` e `next`, e no host a largura de ponteiro os separa. Corrigido sob
  `MELEE_HOST`, andando pelo próprio JObj.
- Com a correção, `gm_Scene_Vs_OnEnter` termina e a cena entra no laço de
  frames (`gm_801A4D34`). O primeiro frame roda os procs e, no desenho,
  `ftDrawCommon_80080E18` → `ftLib_80086A8C` → `Camera_80030CFC` projeta um
  ponto da caixa de câmera do lutador fora de ±50.000 (assert de
  `lbvector.c:383`). A causa era a câmera: `Camera_ApplyQuake` lê a descrição
  da câmera (`cm_803BCB64`) pelo layout de statics em sequência a partir de
  `cm_803BCB18`, que no host não vale; a translação de tremor saía NaN e
  contaminava olho e interesse. Corrigido sob `MELEE_HOST`, lendo
  `cm_803BCB64` direto.
- Com a câmera corrigida o primeiro frame desenha, e os procs dos frames
  seguintes acharam mais dois pontos que dependem do layout de 32 bits: as
  listas de geradores de partícula guardam endereços em `u32`
  (`hsd_804D78F8` e `hsd_804D78F4`), e o HUD lê o `IfDamageState` por outra
  struct com fillers nos offsets do console (`UnkX`). Os dois corrigidos sob
  `MELEE_HOST`.
- Depois deles o jogo abortava no `malloc` da glibc, sinal de heap corrompido
  antes. A mesma rota sob ASan achou primeiro mais duas visões de statics em
  sequência (`lbRefract_800222A4` e `ftmaterial.c`), também corrigidas, e
  depois a escrita que corrompia o heap: os segmentos de colisão de
  `mpIsland_8005A728` eram alocados com o tamanho do console (0x2C), menor que
  a struct no host. Corrigido sob `MELEE_HOST` com `sizeof`.
- As rodadas seguintes do ASan acharam mais dois pontos na criação dos
  lutadores: `ft_800852B0` zera caches pela distância entre globais no console
  (`ftData_Table_Unk0`, `ftData_UnkIntPairs`, `ft_8045993C`), e a luz de cada
  lutador (`ftCo_09F4.c`) usa cinco floats no lugar de um `HSD_WObjDesc`. Os
  dois corrigidos sob `MELEE_HOST`.
- Depois, as listas de símbolos dos loaders de `lbarchive.c` terminam num `0`
  literal, que no x86-64 chega a `va_arg` com a metade alta indefinida quando
  vai na pilha. No host as macros de `lbarchive.h` juntam os argumentos num
  vetor de ponteiros, onde o `0` vira nulo.
- Com isso a rota sob ASan passa por toda a entrada da cena e chega aos procs
  dos frames, onde achou `lbVector_WorldToScreen` passando uma matriz 3x4 a
  `MTXPerspective` (que escreve 4x4) e `ifstatus.c` convertendo 255 em float
  direto para `s8`. Os dois corrigidos sob `MELEE_HOST`.
- A rodada seguinte já tinha os lutadores em `Fighter_procUpdate` e achou o
  pool de `HSD_psAppSRT` criado com o tamanho do console (0xA4, 184 bytes no
  host). Corrigido sob `MELEE_HOST` com `sizeof`.
- Bloqueio seguinte, resolvido: com esse lote, a rota da luta sob ASan
  (`GS_VS` só local) rodava os procs dos lutadores até `Fighter_8006A360` →
  `ftCo_RebirthWait_Anim` → `ftCo_8008A7A8` → `ftAnim_8006EBA4` →
  `ftAction_80073240`, e o `Command_04` de `lbcommand.c:57` lia um endereço
  inválido (SEGV). O comando nem devia rodar: `ftAction_80073240` lê o opcode
  por `gmScriptEventDefault` (`ft/types.h`), bit-fields fora de `lb/types.h`
  que o gerador de layouts não cobre, e o host tirava o opcode dos seis bits
  baixos da palavra. Corrigido sob `MELEE_HOST` com os campos invertidos.
- Com o opcode certo, a luta roda o laço de frames sem erro: 600 frames da
  luta em ~60 s no `host-debug` e 400 s sob ASan, sem terminar sozinha.
- Bloqueio seguinte, resolvido sem código do host: a rota não tinha fim, e
  o próprio jogo dá a saída. O HUD libera a pausa no frame 655; START pausa e
  L+R+A+START encerra a luta como no contest (`fn_8016CF4C` →
  `gm_801A4B60`). `gm_Scene_Vs_OnExit` roda, o modo VS do host volta à CSS e B
  segurado leva ao menu. `GS_VS` entrou na tabela do host e a rota virou o
  teste `melee-host-vs-match-asset` (luta de 175 frames).
- Executado: imagem da luta. `--run-modes` grava um frame pedido
  (`FRAME:BMP=arquivo`) pelo presenter escondido. O frame 640 da rota do teste
  mostra o estágio, o "Go!", o cronômetro, P1 e P2 e os painéis de dano; o 695
  mostra o menu de pausa. Detalhes em `docs/native_port_status.md`.
- Resolvido: o letreiro de início saía como quadriláteros brancos porque a
  textura recebia a paleta de outro draw. O frame é lido depois do último
  draw, e a paleta era pedida pelo nome (`GX_TLUT0`); o recorder GX passou a
  guardar a paleta de cada draw. O "Ready" e a contagem aparecem.
- Resolvido: a câmera descia porque os lutadores caíam. Sem `coll_data`,
  `mpLibLoad` usava um mapa vazio; medido sob gdb, os dois caíam de y 22 a
  -201 entre os frames 340 e 440 do modo, com a câmera atrás. Com o tradutor
  de `coll_data` eles pousam e a câmera fica no estágio. O pouso passou por
  dois pontos de host, em `lbanim.c` e `lb_020A.c`. Detalhes em
  `docs/native_port_status.md`.
- Resolvido: os lutadores não eram desenhados porque `fighter.c` liga a flag
  de desenho gravando `byte = 1` num union de `u8` com bit-fields
  (`UnkFlagStruct`), e o MWCC e o GCC alocam esses bits em ordem contrária. O
  host declara os bits invertidos.
- Resolvido: os planos enormes eram vértices sem a translação da câmera. O
  recorder GX do host guardava a matriz de normal nas linhas da matriz de
  posição, e o HSD carrega a inversa transposta, sem translação, logo depois
  da posição de todo PObj iluminado. Com as matrizes de normal à parte, como
  no GX, os dois Fox aparecem no tamanho certo sobre o estágio.
- Medido: a faixa preta é a sombra projetada dos lutadores. O passo de sombra
  copia o que desenhou para uma textura de 4 bits com `GXCopyTex`, que o host
  só registra, e a textura fica com o conteúdo de uma alocação não zerada.
  Encher a cópia de branco, num experimento local, apagou a faixa.
- Próximo bloqueio: produzir as cópias de EFB no host. `GXCopyTex` precisa
  rasterizar o que foi desenhado no alvo da cópia desde o começo do passo e
  gravar no formato pedido. Para a sombra, isso é fundo branco e silhueta em
  cinza, sem textura, em projeção ortográfica, gravados em `GX_CTF_R4`;
  `lbrefract.c` e `tobj.c` também copiam. Depois: ver os lutadores
  responderem ao stick e aos botões, conferir a SSS (saía quase toda azul
  antes da correção de paleta e não foi olhada depois) e os dados de estágio
  que faltam (`itemdata`, `ALDYakuAll`, `yakumono_param`, `map_plit`,
  `quake_model_set`). A tela de resultados (`onExitVs`) continua fora.
- Executado em commits de 14/09/2026: a copia I4 da sombra, o stick deslocando
  P1 e o botao A tirando P1 do `Wait`, todos conferidos pela rota do teste.
- Resolvido: segurar o stick travava a luta no frame 670 com o processo
  crescendo sem fim. O script da corrida girava porque a animacao nao repetia:
  as flags de cada acao sao gravadas inteiras em `fp->x594_s32` e lidas por
  bit-fields que o MWCC conta do bit mais alto. O host declara esses bits a
  partir do menos significativo. Detalhes em `docs/native_port_status.md`.
- Executado: a tela de resultados, pelo codigo do jogo. A rota do teste passa
  por CSS, SSS, luta, resultados (406 frames) e volta a CSS, e B leva ao menu.
  Saindo dela, o jogo ia para o aviso de premio por um trofeu concedido com
  lixo: `fn_80166A8C`, a conversao de float para `u16` pelo fast cast do SDK,
  so existe em assembly e nao gravava nada no host. Corrigido; detalhes em
  `docs/native_port_status.md`.
- Executado: o especial neutro do Fox. B parava com nome no blaster, cujos
  atributos proprios o host nao traduzia; o tradutor de `ftDataFox` passou a
  traduzir os tres itens do Fox, que em `PlFx.dat` so tem floats.
- Medido: o KO roda pelo jogo. P1 pula a parede do lado esquerdo (que em
  runtime fica a 0,9 vezes as coordenadas do arquivo, transformada pelo joint
  do mapa), cai, entra em `DeadDown` e renasce na plataforma. Proximo passo: o
  KO na rota do teste e uma luta levada ate o fim.
- Executado: uma luta levada ate o fim. Na CSS o menu de regras, que parava
  num JObj de endereco cortado por `mn_80231634`, passa as regras a estoque 1;
  P1 cai do estagio, a luta termina por eliminacao e os resultados de uma luta
  concluida voltam a CSS. Teste `melee-host-vs-stock-match-asset`; detalhes em
  `docs/native_port_status.md`. Proximo passo: olhar os frames dessa rota (o
  "GAME!", o vencedor, a SSS) e medir o ritmo num build otimizado, porque o
  `host-debug` fica em 4 a 5 frames por segundo.
- Como a tela de resultados funciona no host. Com a tabela de estados do console e
  `GS_RESULTS` na tabela de cenas, a luta cancelada entra em
  `gm_Scene_Results_OnEnter` pelo proprio jogo (`fn_8016CF4C` com
  `OUTCOME_NO_CONTEST`), e a cena roda seus frames: no proc `fn_80179350`, `x1`
  vai de 0 a 3 em cerca de 120 frames, com duas paginas. No estado 3
  (`fn_80178050`) cada jogador humano precisa apertar START na propria porta;
  uma porta desconectada conta como pronta. A entrada passou por tres leituras
  de estaticos em sequencia (camera, `ResultsDisplayLayout` e
  `ftMapping_list`), corrigidas; detalhes em `docs/native_port_status.md`.
  Numa luta que nao foi cancelada, o Fox de demo pode criar o blaster (item
  74), cujos atributos proprios o host ainda nao traduz.
- Levantado para o tradutor de `ftData*` (medido em `PlFx.dat` e nos 58
  arquivos de personagem em 14/09/2026):
  - `ftDataFox` tem 24 campos, todos preenchidos menos `x28`. Só escalares:
    os atributos comuns (`ftCo_DatAttrs`, 0x184 bytes, 82 membros, só o último
    é `u8`), `x24` (um `WaitStruct` de inteiros, que `ftCo_8008A7A8` recebe),
    `x34`, `x38`, a câmera (`x3C`), `itPickup` (`x40`), `x50` (um `Vec2` que
    `ftchangeparam.c` copia) e os limites de borda de `x44` (com seis `s16`).
  - Os atributos próprios (`x4`) mudam de layout por personagem. Os do Fox
    (`ftFox_DatAttrs`, 0xD4 bytes) são 44 escalares e um `ReflectDesc` que
    termina num `u8`.
  - As tabelas de ação (`xC` e `x14`) têm 327 e 14 entradas
    `Fighter_WaitAnimData` de 0x18 bytes: nome, offset e tamanho da animação em
    `PlFxAJ.dat`, script de comando e flags. `x10` e `x18` são pares de `u8`
    por ação. Nos 58 arquivos de personagem, os 10.091 scripts obedecem à regra
    da conversão de comandos, menos 52 ponteiros nos 14 arquivos de cópia do
    Kirby (`ftDataKirbyCopy*`), que têm outro formato.
  - Partes (`x8`): `FtPartsDesc`, com `vis_table` em linhas por fantasia de
    quatro registros `{contagem, ponteiro}` (formato ainda a confirmar), e
    `ftData_x8_x8`, com uma tabela de pares `u16`. `x1C` são cinco
    `ftData_x1C` com vetor de bytes de partes e vetor de animações; `x20` é um
    vetor em que só o índice 2 é ponteiro de joint (os outros valem 0 e 8);
    `x5C` é o joint do metal.
  - Com ponteiros e formato próprio: a dinâmica (`x2C`, com
    `ArticleDynamicBones` e `FigaTree`), as hurtboxes (`x30`,
    `ftHurtboxInit`), os itens do personagem (`x48_items`, quatro `Article`
    como os de `itPublicData`), os SFX (`x4C`, `FtSFX` com `FtSFXArr`) e o IK
    (`x58`, `ftData_x58_t`, índices `u8` e comprimentos `f32`).
  - `x54` é declarado `int`, mas no disco é um ponteiro relocado e
    `ftCo_09F7.c` o lê como `int*`. No host o campo precisa de tipo ponteiro:
    como `int`, o endereço seria cortado e `x58` ficaria no offset errado.
  - Depois de `ftData_8008572C`, `Fighter_Create` carrega a fantasia
    (`ftData_80085820`) e as animações. `ftData_80085A14` grava em `x14` de
    cada ação o endereço da animação dentro de `PlFxAJ.dat`, que fica em ARAM,
    e `ftData_80085E50` copia a animação pedida com uma leitura síncrona de
    ARAM (`lbArq_80014BD0`) antes de parseá-la. Quando outro lutador já tem a
    mesma animação, a cópia vem dele e passa por `lbArchiveRelocate`.
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
