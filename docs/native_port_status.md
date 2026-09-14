# Status do port nativo

Atualizado em 13 de setembro de 2026.

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
- [x] Heap original `OSAlloc` rodando sobre memoria do host. `OSAlloc.c` e
  `OSArena.c` da SDK compilam nativamente, com enderecos carregados em
  largura de ponteiro sob `MELEE_HOST`. As macros `ROUND`, `TRUNC` e `OFFSET`
  de `os.h` receberam a mesma protecao. Foi verificado que, com
  `unsigned long` de 32 bits (a largura do PowerPC), o codigo gerado de
  `OSAlloc.c` e identico ao de antes da mudanca, entao o caminho matching nao
  muda.
- [x] Fachada `melee_host_os_heap_*` que cria a arena do host e um heap sobre
  ela, repetindo a sequencia do boot. O heap entrega blocos alinhados a 32
  bytes em enderecos reais acima de 4 GB, que a aritmetica de 32 bits original
  teria truncado.
- [x] `OSGetPhysicalMemSize` e `OSGetConsoleSimulatedMemSize` reportam os 24 MB
  do console, nao a RAM do host: o jogo os usa para decidir quanto consumir.
- [x] Camada de objetos graficos HSD compilada e ligada nativamente: `cobj.c`,
  `lobj.c`, `jobj.c`, `dobj.c`, `mobj.c`, `tobj.c`, `pobj.c`, `robj.c`,
  `wobj.c`, `fog.c`, `displayfunc.c` e `shadow.c`, junto de `state.c`,
  `tev.c`, `texp.c`, `texpdag.c`, `bytecode.c`, `perf.c`, `util.c`,
  `video.c` e `initialize.c`. Os substitutos que abortavam foram removidos: um
  GObj que possui um JObj agora e liberado pelo destrutor real da classe, que
  e o caminho que `ground.c` e `fighter.c` tomam.
- [x] `MTXLightFrustum`, `MTXLightPerspective` e `MTXLightOrtho`, as projecoes
  que mapeiam espaco de olho direto para coordenada de textura, usadas pelas
  sombras projetadas.
- [x] `GXGetTexBufferSize` com o rodape de bloco de cada formato GX e a soma
  dos niveis de mipmap.
- [x] Camada VI do host em `port/src/video/vi.cpp`: contador de retrace,
  paridade de campo, callbacks pre e pos retrace na ordem original e latch de
  registradores sombra no retrace seguinte ao `VIFlush`. Nada dorme nem le
  relogio de parede: o tempo avanca quando o jogo bloqueia em
  `VIWaitForRetrace` ou quando o laco do host chama
  `melee_host_video_advance_retrace`.
- [x] Materializacao de descritores HSD em layout host. O arquivo guarda os
  descritores como o PowerPC os viu: escalares big-endian e campos de ponteiro
  de 32 bits que `archive.c` realoca no lugar. O host nao pode fazer isso,
  porque cada ponteiro precisaria de oito bytes onde o arquivo tem quatro e a
  relocacao sobrescreveria o campo seguinte. `port/src/assets/hsd_materialize`
  aloca cada descritor de novo em layout host e preenche seus ponteiros com
  enderecos reais: `HSD_Joint`, `HSD_DObjDesc`, `HSD_MObjDesc`, `HSD_Material`,
  `HSD_PEDesc`, `HSD_TObjDesc`, `HSD_ImageDesc`, `HSD_TlutDesc`,
  `HSD_TexLODDesc`, `HSD_TObjTevDesc`, `HSD_PObjDesc`, `HSD_VtxDescList`,
  `HSD_ShapeSetDesc`, `HSD_EnvelopeDesc` e `HSD_RObjDesc`.
- [x] Os payloads GX nao sao traduzidos. Display lists, arrays de vertice,
  imagens e paletas mantem os bytes big-endian, porque e nessa ordem que o
  interpretador de display list e os decodificadores de textura os leem. Eles
  vivem em uma copia verbatim da secao de dados, alinhada a 32 bytes, e por
  isso ponteiros para dentro deles nao precisam de tamanho.
- [x] Os descritores saem de um unico bloco contiguo. E isso que mantem
  `jobj->id = (u32) joint` utilizavel: o original indexa a tabela de IDs pelo
  endereco truncado do joint, e o truncamento continua injetivo enquanto todos
  os joints compartilham os mesmos 32 bits altos. As referencias de skin rigido
  e de envelope dependem dessa tabela.
- [x] Campo de ponteiro que o arquivo nao relocou precisa ler zero. Um valor
  nao nulo sem relocacao e recusado, em vez de chegar aos loaders originais
  como ponteiro selvagem. A contagem de blocos de display list da o tamanho
  exato, entao um arquivo truncado e detectado antes do interpretador.
- [x] Cena HSD real carregada do disco pela camada de objetos graficos
  original. `HSD_JObjLoadJoint` e a camada abaixo dela constroem a arvore de
  JObj, DObj, MObj, TObj e PObj a partir dos descritores materializados. Os
  647 simbolos de cena e de joint do disco carregam: 13.395 JObjs e 22.541
  PObjs, incluindo modelos de personagem com envelope e shape set (o joint do
  Mario tem 61 JObjs e profundidade 13) e cenas com ate doze modelos.
- [x] Fachada C ABI `melee_host_scene_graphics_*` com handles opacos de 32
  bits, mais os diagnosticos `melee-pc --load-scene FILE SYMBOL [MODEL_INDEX]`
  e `melee-pc --load-joint FILE SYMBOL`. A liberacao passa pelos destrutores
  originais e devolve os vetores e matrizes agrupados, inclusive a matriz de
  envelope que o loader toma para cada joint que tem uma.
- [x] `texp.c` tomava o endereco de um membro de ponteiro nulo para terminar a
  lista de estagios TEV, o que funciona porque `desc` e o primeiro membro. Sob
  `MELEE_HOST` isso e escrito de forma explicita; sem o define a unidade de
  traducao continua identica byte a byte, verificado pelo pre-processador.
- [x] Cena real desenhada pelo caminho de render original. `HSD_JObjDispAll`
  percorre a arvore carregada nas tres passagens que o callback de render do
  jogo usa (opaca, texture-edge, translucida), passa por `HSD_JObjDispDObj`,
  `HSD_JObjDispSub`, `HSD_DObjDisp` e `HSD_PObjDisp`, e as display lists do
  arquivo chegam ao recorder GX do host. Os 647 simbolos do disco desenham:
  3.368.616 triangulos e 3.836.934 vertices, sem um unico erro de display list
  e sem um unico indice recusado.
- [x] Contagem cruzada entre o caminho de render e o schema de leitura
  separado. Nas cenas de um modelo as duas rotas independentes dao o mesmo
  numero de triangulos e vertices em 9 casos, e nos 7 restantes a diferenca e
  inteiramente de objetos escondidos: o caminho original nao desenha JObj nem
  DObj com flag de oculto, nem PObj que descarta as duas faces, enquanto o
  schema percorre tudo. As estatisticas de carga passam a relatar isso
  (`not drawn`), entao a divergencia se explica sozinha.
- [x] `GX_VA_NBT` passou a ser consumido na posicao do normal dentro da display
  list, que e onde o hardware o coloca, e nao na posicao do seu numero de
  `GXAttr` (25, depois das coordenadas de textura). A ordem numerica lia o
  indice de textura como normal e dessincronizava todo vertice seguinte. Isso
  so aparecia com asset real, porque os testes sinteticos tinham apenas
  posicao e NBT, onde as duas ordens coincidem. Foi o que travava os modelos de
  trofeu.
- [x] Regiao de array declarada ao host. O `GXSetArray` original nao carrega
  tamanho, entao o limite que o preview tinha se perdia quando o codigo
  original passou a dirigir o GX. O materializador declara a extensao da sua
  copia do payload e `GXSetArray` deriva o limite dela: no console o array
  corria para o que viesse depois no arquivo, e o host para no fim do arquivo
  em vez de ler alem dele.
- [x] Indices recusados sao contados, nao ignorados. Quando um indice cai fora
  do array o atributo e descartado, o que sem contador pareceria um vertice que
  simplesmente nao tinha normal. `melee_host_gx_rejected_index_count` expoe
  isso, e foi esse contador que mostrou que o limite de array estava mascarando
  a dessincronizacao de NBT em vez de corrigi-la.
- [x] Modo de video instalado antes do render. `setupNormalCamera` escala o
  viewport pela razao entre framebuffer e VI do render mode, entao sem um
  instalado a conta virava NaN e a conversao para inteiro era undefined
  behavior. A fachada instala `GXNtsc480IntDf`, o mesmo que `gmMain` escolhe.
- [x] Duas construcoes de codigo original que os sanitizers acusam ficaram
  explicitas sob `MELEE_HOST`: `cobj.c` desloca 1 para o bit de sinal de um
  `int` e `texp.c` toma o endereco de um membro de ponteiro nulo para terminar
  uma lista. Sem o define as duas unidades de traducao continuam identicas byte
  a byte, verificado pelo pre-processador com o `-DMUST_MATCH` que o build
  matching usa.
- [x] Fachada `melee_host_scene_graphics_render` e os diagnosticos
  `melee-pc --render-scene` e `melee-pc --render-joint`.
- [x] Vertices capturados passam pela matriz de posicao que o GX tinha em
  vigor, e nao por uma transformacao aplicada depois. Isso inclui
  `GX_VA_PNMTXIDX`, o indice que nomeia uma matriz por vertice: e assim que um
  PObj com envelope enderecau uma junta diferente em cada vertice, o que
  nenhuma transformacao unica por PObj reproduz. A normal usa a inversa
  transposta normalizada, e tangente e binormal a transformacao direcional.
- [x] A memoria de matrizes do host sabe se cada linha foi carregada. Ela
  reseta em zeros, nao em identidade, entao transformar por uma linha intocada
  colapsaria a geometria na origem; sem carga a posicao fica onde estava.
- [x] Texturas capturadas pelo caminho original. O que o
  `GXLoadTexObj` deixou em texmap 0 e resolvido em uma tabela de texturas
  distintas, e cada vertice carrega o indice dela. O consumidor faz upload uma
  vez por imagem em vez de resolver por triangulo.
- [x] `HSD_CObjDesc` materializado, com `HSD_WObjDesc` de olho e de interesse,
  tipo de projecao, viewport e scissor. Todas as cenas do disco passam a
  desenhar pela camera do proprio asset; a camera substituta do host ficou
  como fallback para simbolo de joint solto, que nao tem cena.
- [x] O espaco da captura e escolha de quem chama. `MELEE_HOST_SCENE_VIEW_WORLD`
  pede uma view identidade ao caminho de display, o que deixa as matrizes que
  ele carrega no GX como transformacoes de mundo puras; `..._SCENE_CAMERA` faz
  o que o jogo faz e entrega espaco de vista.
- [x] Preview SDL/OpenGL alimentado pelo caminho de display original, nao mais
  pelo schema de leitura separado: `melee-pc --view-scene FILE SYMBOL` e
  `--view-joint FILE SYMBOL`. O joint do Mario rende 6.328 triangulos com 32 de
  32 texturas decodificadas, e `GmPause.dat` 130 triangulos com 9 de 9.
- [x] Estado de pixel capturado por draw. Cada draw registra o estado que o
  GX tinha em vigor - culling, teste e escrita de profundidade com a funcao de
  comparacao, modo e fatores de blend, compare de alpha e mascaras de cor e
  alpha - em uma tabela de estados distintos, e cada vertice carrega o indice
  dela. E assim que um consumidor agrupa triangulos que compartilham estado em
  vez de supor passes fixos.
- [x] O preview obedece a esses estados. Ele nao tem mais dois passes fixos:
  monta um grupo por estado capturado, desenha primeiro os que nao fazem blend
  para que a geometria opaca escreva profundidade antes, e aplica culling,
  profundidade, blend, compare de alpha e mascara de cor de cada grupo. A
  correspondencia de fatores de blend respeita o lado em que o fator e usado,
  que e o motivo de `GX_BL_SRCCLR` e `GX_BL_DSTCLR` terem o mesmo valor no
  enum.
- [x] Reducao das duas comparacoes de alpha do GX a uma. Uma comparacao
  verdadeira para todo alpha nao informa nada: `GX_ALWAYS`, mas tambem
  `<= 255` e `>= 0`, que e como o jogo escreve um lado aberto. Sob `GX_AOP_AND`
  esse lado cai. O disco inteiro usa exatamente duas combinacoes,
  `GREATER@0 AND GREATER@0` e `GEQUAL@102 AND LEQUAL@255`, e as duas reduzem
  sem perda. A reducao vive junto do modelo GX, nao do renderer, e tem teste
  proprio.
- [x] Levantamento dos estados reais do disco: 24 estados distintos nos 647
  simbolos, no maximo oito por simbolo. Nenhum usa `GX_BM_LOGIC` nem
  `GX_CULL_ALL`, os dois casos que o preview nao modela.
- [x] Programa TEV capturado por draw. HSD compila as expressoes de material em
  estagios TEV, entao nada vem de preset de `GXSetTevOp`: todo estagio do disco
  e custom, e o programa precisa ser lido para saber o que faz. Cada
  configuracao distinta entra em uma tabela e cada vertice carrega o indice
  dela.
- [x] A deduplicacao ignora os componentes que o programa nao le.
  `HSD_TExpSetReg` monta os valores de registrador em um `GXColor reg[8]` local
  **nao inicializado** e escreve so os componentes que a expressao nomeia, entao
  o resto do que estava na pilha chega ao GX. Isso nao afeta a imagem, porque
  nenhum estagio le esses componentes, mas fazia duas draws do mesmo material
  parecerem diferentes: o joint do Mario reportava 39 programas distintos no
  build de debug e 37 ou 38 no de sanitizers, variando entre execucoes. Marcando
  quais componentes de registrador e de konst cada estagio realmente le, o mesmo
  modelo reporta **4** programas, igual nos dois builds e estavel entre
  execucoes. O codigo original nao foi alterado: no console ele le a mesma
  sujeira de pilha.
- [x] TEV avaliado por fragmento. `port/src/gx/tev.cpp` executa um programa
  capturado como o hardware combina: entradas a/b/c de 8 bits e d de 11 bits
  com sinal, o lerp que estica c de 255 para 256, arredondamento de 128 na soma
  e 127 na subtracao, bias e escala antes do deslocamento, clamp em 0..255 ou
  -1024..1023, os quatro modos de comparacao (R8, GR16, BGR24, RGB8/A8),
  konst por fracao, cor inteira ou componente, as swap tables do `GXInit` e os
  registradores que um estagio escreve servindo aos seguintes. A mesma
  semantica gera o GLSL: a estrutura do programa vira codigo e registradores e
  konst viram uniforms, entao o texto do shader e a propria chave do cache. As
  duas alpha compare e a logica que as combina rodam no shader, sem reducao.
  Substitui a reducao simbolica anterior (`resolve_shading`), que so lia um
  estagio e aproximava 80% dos triangulos.
- [x] Referencia de CPU testada contra a formula do hardware (lerp e
  arredondamento, bias/escala/subtracao com e sem clamp, registrador de 11 bits
  entre estagios, comparacoes empacotadas, konst, textura ausente, canal nulo e
  swap), e GLSL conferido contra ela na GPU: `melee-pc --tev-conformance-scene`
  e `--tev-conformance-joint` desenham cada par de programa e estado de pixel
  capturado num alvo de um pixel, com entradas aleatorias, e exigem o mesmo
  pixel, descarte incluido.
- [x] Captura do que o TEV le. Todo `GX_VA_TEX0..7` do stream e guardado e o
  texgen de `GXSetTexCoordGen2` roda por vertice: a fonte (posicao, normal,
  binormal, tangente ou coordenada crua) passa pela matriz de textura, e
  normalizada quando pedido e passa pela matriz pos-transformacao, com s/t/q
  para a divisao acontecer por fragmento. `GX_VA_TEXnMTXIDX` escolhe a matriz de
  um vertice so, e SRTG le a cor ja iluminada. Cada draw registra um conjunto
  de texturas com a textura de cada mapa que algum estagio amostra.
- [x] Memoria de matrizes com 128 linhas. As matrizes pos-transformacao
  (`GX_PTTEXMTX0` a `GX_PTIDENTITY`) ficam acima de 64, e o HSD guarda nelas
  toda transformacao de textura, pedindo `GX_IDENTITY` na primeira matriz; com
  64 linhas elas eram descartadas e toda UV escalada ou animada saia errada.
- [x] Reset do GX espelha o `GXInit`: ordens TEV dos oito primeiros estagios no
  proprio mapa e coordenada, um texgen 2x4 identidade por coordenada, estagio 0
  em REPLACE, konst 1/4 e canais sem luz com material do vertice sobre registro
  branco. `GXSetTevOp` grava as entradas e operacoes da expansao da SDK, e nao
  so o modo. Com zero canais a cor rasterizada e a do vertice (branca sem ela),
  e com menos de dois o segundo canal repete o primeiro.
- [x] Luzes substitutas no render da fachada: uma ambiente e uma infinita,
  criadas e registradas pelo `HSD_LObj` original na mesma vista da geometria.
  Sem elas todo material iluminado saia preto, porque o ambiente do canal e o
  ambiente do material vezes a luz ambiente corrente.
- [x] Preview SDL/OpenGL reescrito sobre shaders: um draw GL por sequencia de
  triangulos com o mesmo estado de pixel, programa TEV e conjunto de texturas,
  as sequencias com blend depois das opacas preservando a ordem, e
  `MELEE_HOST_SCREENSHOT=arquivo.bmp` renderiza um quadro fora da tela.
- [x] Varredura do disco pelo caminho novo: nos 647 simbolos de cena e de joint
  (690 modelos, 3.400.842 triangulos, zero erro de display list, zero indice
  recusado), **3.342.083 triangulos (98,3%) tem o TEV avaliado por inteiro**;
  os 58.759 restantes, em 55 simbolos, amostram uma coordenada de bump. A
  conformidade rodou nos 678 modelos com geometria: 277.632 casos e **zero
  divergencia** entre GLSL e referencia.
- [x] Materializacao de animacao. `HSD_AnimJoint`, `HSD_MatAnimJoint`,
  `HSD_ShapeAnimJoint`, `HSD_AObjDesc`, `HSD_FObjDesc`, `HSD_MatAnim`,
  `HSD_TexAnim` com suas tabelas de imagem e de paleta, `HSD_RenderAnim`,
  `HSD_ChanAnim`, `HSD_TevRegAnim`, `HSD_ShapeAnimDObj`, `HSD_ShapeAnim` e
  `HSD_RObjAnimJoint`. Os streams de keyframe do `HSD_FObjDesc` nao sao
  traduzidos: como as display lists, mantem os bytes big-endian que o
  interpretador original le, e o campo `length` da o tamanho exato, entao o
  intervalo inteiro e validado na materializacao em vez de durante a leitura.
- [x] As tres tabelas de animacao que um modelo de cena carrega em
  `DynamicModelDesc` (`anims`, `matanims`, `shapeanims`), que e como uma cena
  nomeia varias animacoes para o mesmo modelo.
- [x] Animacao real executada pelo codigo original. `HSD_JObjAddAnimAll`
  percorre as arvores ao lado da arvore de objetos, `HSD_AObjLoadDesc` e
  `HSD_FObjLoadDesc` constroem os objetos e `HSD_JObjAnimAll` avanca um frame
  por chamada. Em `GmTtAll.dat` o modelo `TtlMoji_Top` recebe 30 AnimJoints e
  30 MatAnimJoints, 65 AObjDesc e 154 FObjDesc com 47.198 bytes de keyframes;
  22 AObj ficam presos na arvore, o contador chega a frame 199 de 1600 e a
  junta 28 anda 51,8 unidades. A primeira interpretacao usa taxa zero por
  causa de `AOBJ_FIRST_PLAY`, que e por isso que N chamadas avancam N-1
  frames.
- [x] Varredura de animacao no disco: **288 de 288** pares de modelo e animacao
  materializam e rodam sem um unico erro, somando 1.136 AnimJoints, 1.189
  AObjDesc, 2.414 FObjDesc e 128.823 bytes de keyframe. Em 36 deles alguma
  junta se move; nos demais a animacao e de material, que muda cor e textura
  sem mexer no esqueleto.
- [x] A mensagem de campo de ponteiro nao relocado passou a dizer o offset e o
  valor. Foi ela que apontou o erro exato quando os dados sinteticos de teste
  estavam mal montados.
- [x] Leitor de arquivos HSD concatenados. Alguns arquivos do jogo sao varios
  arquivos HSD enfileirados, cada um alinhado a 32 bytes: um personagem guarda
  assim uma animacao por acao. Nao ha indice; o header de cada um declara o
  proprio tamanho, e e isso que torna a caminhada possivel. `PlMrAJ.dat` tem
  195 membros, e o disco tem 6.271 animacoes em 59 arquivos.
- [x] `FigaTree` e `FigaTrack` materializados. A animacao de personagem nao usa
  as arvores HSD: usa o formato proprio do Melee, plano, com uma lista dizendo
  quantas tracks cada osso consome (terminada em -1) e as tracks enfileiradas.
  A lista de nos e de bytes com sinal, entao e usada onde esta; os streams de
  keyframe seguem a mesma codificacao do `HSD_FObjDesc` e mantem os bytes
  originais com o tamanho declarado por track.
- [x] `lbanim.c` compilado nativamente. `lbAnim_8001E6D8` aplica um FigaTree
  direto a um `HSD_JObj`, sem precisar de `Fighter`, o que permite acionar o
  esqueleto antes do runtime de luta existir.
- [x] Esqueleto de personagem animando. `PlyMario5K_Share_ACTION_WalkMiddle`
  anexa a 48 das 61 juntas do Mario, com 2.150 bytes de keyframe, e em 45
  frames **58 de 61 juntas se movem**, a maior andando 6,88 unidades.
- [x] Varredura de animacao de personagem: **6.245 de 6.245** animacoes, em 33
  personagens, anexam e movem juntas, sem um unico erro.
- [x] `fobj.c` deslocava um `s8` negativo para a esquerda ao montar um valor de
  16 bits, que e undefined behavior. Sob `MELEE_HOST` o deslocamento passa por
  tipo sem sinal, com o mesmo resultado numerico (verificado: as posicoes das
  juntas nao mudaram em nenhuma casa decimal). Sem o define a unidade de
  traducao continua identica, verificado com o `-DMUST_MATCH` do build
  matching.
- [x] Referencias de `HSD_AObjDesc.obj_id` para JObj. O materializador resolve
  o ponteiro relocado do disco para o `HSD_Joint` materializado e grava a chave
  de 32 bits que o ID table original usa; `HSD_AObjLoadDesc` encontra o JObj
  ja carregado, toma a referencia e `HSD_AObjRemove` a devolve. Assim nenhum
  endereco PPC e convertido em ponteiro nativo. O teste integrado tambem
  cobre a liberacao explicita de uma referencia ciclica sintetica antes de
  desmontar a arvore.
- [x] Avaliacao host dos canais de iluminacao GX. Cada vertice capturado
  recebe `COLOR0A0` e `COLOR1A1` separados, calculados a partir de ambiente,
  material, normal, luzes, difuso e atenuacao do estado GX; os dois chegam ao
  shader como cores rasterizadas, e cada estagio escolhe a sua pela ordem TEV.
- [x] Fronteira deterministica de frame para o runtime de cena: executa
  `HSD_GObj_RunProcs`, entrega uma fence `GXDrawDone` pendente e por fim avanca
  um retrace VI quando o video ja foi inicializado. A ordem e coberta por teste
  e conserva o bootstrap headless, que nao cria video implicitamente.
- [x] `synth.c` no build nativo. Os callbacks de DevCom usam o argumento de
  largura de ponteiro no host, eliminando a primeira incompatibilidade de
  assinatura em 64 bits; `HSD_Synth_804D6018`, `HSD_AudioMalloc` e
  `HSD_AudioFree` passam a vir do modulo original. Antes de AX/ARAM existir,
  o allocator host preserva alinhamento de 32 bytes para que DevCom continue a
  inicializar com seguranca. `HSD_SynthInit` tambem executa contra uma fachada
  deterministica de AX/AI e offsets ARAM alinhados; a limpeza ARAM inicial e
  redundante nesse espaco virtual zerado e nao agenda DMA. O caminho de
  voz/DSP ainda nao e acionado nem produz audio.
- [x] API de arquivo HSD do sysdolphin implementada pelo host
  (`port/src/assets/hsd_host_archive.cpp`): `HSD_ArchiveParse`,
  `HSD_ArchiveGetPublicAddress`, `HSD_ArchiveGetExtern` e
  `HSD_ArchiveLocateExtern`, com as assinaturas originais, no lugar de
  `archive.c`. O original reloca o arquivo no lugar e devolve um ponteiro para
  dentro dele, o que em 64 bits sobrescreveria o campo seguinte. Aqui o parse
  valida o arquivo, e o simbolo publico e reconstruido em layout host pelo
  materializador na primeira vez que e pedido. O arquivo nao diz o tipo de um
  simbolo; o jogo sabe pelo nome que pede, e o host le o mesmo do sufixo
  (`_joint`, `_animjoint`, `_matanim_joint`, `_shapeanim_joint`, `_camera`,
  `_scene_lights`, `_fog`, `_sobjdesc`, `_figatree`, `_scene_data`,
  `_scene_models`). Sufixo
  sem traducao e
  recusado com relatorio, em vez de devolvido como ponteiro para bytes
  big-endian. O mesmo simbolo pedido duas vezes devolve o mesmo descritor.
- [x] A identidade do arquivo e o buffer, nao o `HSD_Archive`: `ftdata.c` faz o
  parse de cada acao num `HSD_Archive` na pilha e continua usando o resultado
  depois que o quadro some. Os descritores vivem ate o mesmo buffer ser
  parseado de novo, que e quando o console sobrescreveria os bytes de onde
  vieram. As tabelas big-endian do cabecalho ficam NULL no `HSD_Archive`, para
  que nada fora da camada as leia como se fossem nativas; `data` continua
  sendo `src + 0x20`, porque `lbArchive_80016EFC` libera `data - 0x20`.
- [x] Externs. `lbArchive_InitializeDAT` resolve todo extern para NULL logo
  depois do parse, e a cadeia de referencias passa pelos proprios campos de
  ponteiro. O host percorre a cadeia e declara esses campos nulos ao
  materializador, que antes os recusaria como ponteiro nao relocado com valor.
- [x] Novos descritores no materializador: camera publica, tabela de luzes
  (`HSD_LightDesc` por tipo, lendo posicao, interesse e o union de parametros
  so para os tipos que `LObjLoad` le, com `HSD_LightPointDesc`,
  `HSD_LightSpotDesc` ou `HSD_LightAttn` conforme as flags de atenuacao, e
  `HSD_LightAnim` com animacao de posicao e de interesse), fog
  (`HSD_FogDesc` e `HSD_FogAdjDesc`) e sprite (`HSD_SObjDesc`, imagem e
  paleta). O `LightList` de `sc/types.h` e declarado dentro de `SceneDesc`,
  o que em C++ vira outro tipo; o host usa uma copia de mesmo layout.
- [x] A lista exata de simbolos que `gmTitle_801A1AC0` passa a
  `lbArchive_LoadSymbols`, contra `GmTtAll.usd`: os 12 traduzem, e os joints,
  a camera, as luzes e o fog carregam pelos loaders originais (37 JObjs,
  2 LObjs). Virou o teste `melee-host-title-archive-asset`.
- [x] Varredura do disco pela API de arquivo (`melee-pc --sweep-archives`): dos
  894 arquivos `.dat`/`.usd`, 861 sao um unico arquivo HSD. Neles, **1.509 de
  1.511** simbolos publicos de tipos traduziveis traduzem, e os 725 joints,
  12 cameras, 17 tabelas de luz e 8 fogs traduzidos carregam pelos loaders
  originais (14.212 JObjs e 51 LObjs construidos e liberados). As duas recusas
  sao as tabelas de luz de `TyLight.dat`, cuja posicao e restrita por um joint
  de spline, que o materializador ja recusava. Os outros 5.519 simbolos sao de
  tipos sem traducao e ficam de fora sem erro: 4.191 imagens e paletas soltas
  (`_CMPR_image`, `_image`, `_tlut`, `_tlut_desc`), cerca de 600 de dados de
  estagio (`map_head`, `coll_data`, `grGroundParam`, `itemdata`,
  `ALDYakuAll`, `yakumono_param`, `map_plit`, `quake_model_set`), 58 `ftData*`,
  43 `_scene_data`, 36 `SIS_*` e 18 `_scene_models`.
- [x] `lobj.h` declara a classe de luz como `hsdLobj`, mas `lobj.c` a define
  como `hsdLObj`; nada no jogo usa a grafia do header. O port declara o nome
  real localmente em vez de editar o header.
- [x] Sequencia de memoria do boot executada no host
  (`port/src/game/boot_memory.c`): arena do tamanho da memoria do console,
  `HSD_SetInitParameter`, `HSD_AllocateXFB`, `HSD_AllocateFifo`,
  `HSD_InitComponent`, `lbMemory_8001564C`, `lbHeap_80015F3C` e
  `lbHeap_80015900`, na ordem de `gmMain` seguida do fim do setup de heap de
  uma cena. Ficam criados o heap principal e o de ARAM (2 dos 6 slots do
  lbHeap), que e o estado do console antes de uma cena manter os heaps de cena.
- [x] `lbmemory.c`, `lbheap.c`, `lbfile.c`, `lblanguage.c` e `lbarchive.c`
  compilados nativamente. `lbmemory.c` fazia toda a aritmetica de endereco em
  `u32` e ligava a pilha de handles de heap escrevendo nos offsets PowerPC
  `base + 0x638` a `0x688`; em 64 bits a primeira chamada ja escreveria fora
  dos handles. Sob `MELEE_HOST` os enderecos vao em largura de ponteiro, a
  pilha e ligada por indice e a separacao entre ARAM e RAM usa o teto de 16 MB
  da ARAM no lugar de `0x80000000`; os textos de assert ficam intactos, porque
  `HSD_ASSERT` os grava no DOL. `lbheap.c` guardava o fim de um heap em `s32`.
  Os callbacks de devcom de `lbfile.c` e `lbmemory.c` recebem `HSD_DevComArg`,
  e `lbArchiveRelocate` recusava no host em vez de relocar (hoje parseia de
  novo a copia; ver a entrada de `ftDataFox`). Verificado: as quatro
  unidades, compiladas com `cc -m32 -O2 -DMUST_MATCH` antes e depois a partir
  do mesmo caminho, geram objetos identicos byte a byte.
- [x] `OSRoundUp32B` e `OSRoundDown32B` arredondavam por `u32`, e
  `HSD_AllocateXFB`, `HSD_AllocateFifo` e `HSD_OSInit` passam enderecos por
  elas: `OSInitAlloc` recebia a base da arena com a metade alta zerada. Sob
  `MELEE_HOST` arredondam em largura de ponteiro, como `ROUND` e `TRUNC`; sem
  o define, `initialize.c` e `lbarchive.c` pre-processam identicos.
- [x] ARAM do host como pilha, igual a SDK: `ARFree` devolve o bloco mais
  recente e seu tamanho, e `ARGetSize` reporta os 16 MB.
- [x] `DVDReadPrio` aceita leitura que termina menos de
  `DVD_MIN_TRANSFER_SIZE` (32 bytes) depois do fim do arquivo, como o
  `dvdfs.c` da SDK, e preenche com zero o que no disco seria padding. `lbFile`
  arredonda o tamanho pedido para 32 bytes; com o host recusando, o devcom
  marcava erro, nao chamava o callback e `waitForDisc` girava para sempre.
  `GmTtAll.usd` tem 276.257 bytes, um byte alem de um multiplo de 32.
- [x] `lb_800195D0`, a espera de disco de `lbfile.c`, e uma fachada do host:
  da um passo no escalonador ao qual o DVD esta ligado, que e quando uma
  leitura termina no host. Sem backend ativo entra em panic em vez de girar.
- [x] Tela de titulo pelo carregador do proprio jogo
  (`melee-pc --boot-title-archive`): a chamada `lbArchive_LoadSymbols` de
  `gmTitle_801A1AC0`, com o mesmo arquivo e a mesma lista, le `GmTtAll.usd`
  pelo heap 0 do lbHeap, `lbFile`, a fila devcom e o DVD do host, e os 12
  simbolos resolvem. Os loaders originais constroem 31 JObjs do logo (21 com
  animacao), 6 do fundo (5 com animacao), 2 luzes, a camera e o fog, e o
  sprite do logo tem imagem. Virou o teste
  `melee-host-boot-title-archive-asset`, em processo proprio porque move a
  arena do OS.
- [x] Tela de titulo entrada pelo codigo do jogo: `gm_801A4BD4`, o setup que o
  gerenciador de cenas faz antes de toda cena, e `gm_Scene_Title_OnEnter`,
  depois do boot do `gmMain` (`melee-pc --boot-title-scene`). O relatorio le as
  listas da propria biblioteca de GObj: 9 GObjs (8 com callback de render),
  3 cameras (a de limpeza e a de desenho do titulo, mais a que
  `DevText_CreateCObj` cria para o texto de depuracao), 1 lista de luzes,
  1 fog, 2 modelos, 4 processos, 37 JObjs e 2 LObjs. Virou o teste
  `melee-host-title-scene-asset`.
- [x] O boot segue o `main()` original ate o fim: nivel `Master` sem
  `/develop.ini` (espelho de `gmMain_8015FDA4`, que e `static`),
  `lbAudioAx_8002838C` antes de `lbMemory_8001564C`, `lbDvd_80018F68`,
  `gmMainLib_8015FCC0`, `HSD_SisLib_803A6048(0xC000)` e `gmMainLib_8015FBA4`
  (idioma, regras e o banco de som principal).
- [x] Modulos originais que entraram no build por essa cadeia: `dbinit.c`,
  `gm_1601.c`, `gm_16F1.c`, `gm_1A3F.c`, `gmcameramode.c`, `gmopening.c`,
  `gmopeningmode.c`, `gmtitle.c`, `lb_00B0.c`, `lb_013B.c`, `lb_0195.c`,
  `lbaudio_ax.c`, `lbdvd.c`, `lbmthp.c`, `lbsnap.c`, `lbspdisplay.c`,
  `mn_22EC.c`, `mnname.c`, `mnnamenew.c`, `if_2FF2.c`, `textdraw.c`,
  `textlib_1.c`, `toy.c`, `stage.c`, `sislib.c`, `sobjlib.c`, `hsd_3915.c`,
  `hsd_3924.c`, `hsd_3A64.c`, `hsd_3A76.c`, `axdriver.c` e os efeitos AXFX da
  SDK (`axfx.c`, `chorus.c`, `delay.c`, `reverb_hi.c`, `reverb_std.c`).
- [x] Alarmes do OS no host (`port/src/os/alarm.c`): fila ordenada pelo tempo
  de disparo, periodicos rearmados antes do handler, como no `InsertAlarm` da
  SDK. Disparam onde o host devolve o controle ao jogo: na espera de disco e na
  fronteira de frame. O relogio de frame da cena e um deles:
  `lb_80019628` arma um alarme periodico de 1/60 s que amostra o pad.
- [x] `__OSBusClock` e `__OSCoreClock` sao constantes sob `MELEE_HOST`; fora do
  compilador da build matching eles liam o endereco `0x800000F8`.
- [x] `VA_END_PTR` em `src/Runtime/platform.h`: termina uma lista variadica de
  ponteiros. O jogo escreve `0`, que um callee de 64 bits le de volta como
  ponteiro e que, passado na pilha, pode trazer lixo na metade alta. Expande
  para `0` na build PowerPC e para um ponteiro nulo no host. Aplicado a
  `gmTitle_801A1AC0` e `lb_80014534`.
- [x] Dados que vivem dentro do DOL, e nao num arquivo do disco, lidos do
  `main.dol` extraido pelo usuario pelo endereco que o jogo usa
  (`port/src/assets/dol_image.cpp`): o atlas da fonte SIS (`0x8040CD40`,
  287 glifos de 512 bytes) e o atlas de depuracao (`0x804088B8`, `0x1C00`
  bytes), com os tamanhos de `config/GALE01/symbols.txt`. A build matching
  embute esses bytes por `.inc` gerados; o host nao os compila nem distribui.
- [x] `lbRumbleData` traduzido pela API de arquivo, reconhecido pelo nome
  inteiro: 40 registros de lista de comandos e prioridade.
- [x] Estagios fora do build sao referencias `weak` em `ground.c`, por um
  header forcado (`port/src/game/host_weak_stages.h`), sem editar a decomp: a
  entrada da tabela fica nula, que `Ground_801C06B8` ja trata como estagio sem
  dados.
- [x] Funcoes nao portadas que o link alcanca param com o proprio nome
  (`port/src/game/unported.c`): `ftData_800855C8`, `ftData_8008578C`,
  `efAsync_OnLoad` e `grDatFiles_801C5FC0`, alcancaveis so pelo preload de VS.
- [x] Audio sem mixer: `ARInit`, `ARQInit` e `AIInit` como fachada;
  `AXAcquireVoice` devolve NULL, que o synth trata como vozes esgotadas, e os
  setters de voz so recebem voz entregue por ele. Os nucleos de DSP do reverb e
  do chorus da SDK sao assembly PowerPC e, sob `MELEE_HOST`, param com nome:
  so rodam dentro do callback aux do mixer.
- [x] Carga de bancos de SFX em `synth.c`: o cabecalho SSM e big-endian e e
  convertido antes do teste de espaco. O console monta os descritores de
  amostra sobre o proprio buffer com tamanhos PowerPC; o host mantem a
  contabilidade (callback do jogo e espaco do banco) e nao monta descritores.
  `HSD_SynthSFXBankDeflag` escrevia 32 posicoes alem do vetor de listas de
  grupo, o que no PowerPC cai em `hsd_SynthSFXBank`; no host o banco e nomeado
  direto.
- [x] Tabela de sons `.sem` em `axdriver.c`: big-endian e com ponteiros de 32
  bits relocados no lugar. O host le contagens e offsets, converte a tabela de
  indices de amostra e monta a de fluxos de comando como vetor de ponteiros a
  parte; as outras duas tabelas nao tem leitor e nao sao relocadas.
- [x] `hsd_3A76.c` passava um `Mtx` de tres linhas a `MTXOrtho` fora de
  `MUST_MATCH`; agora e `Mtx44`. `hsd_3915.c` escrevia direto em `GXWGFifo`; sob
  `MELEE_HOST` usa `GXPosition2f32`, que grava os mesmos dois floats.
- [x] Matching das mudancas na decomp e na SDK desta etapa, contra
  `e8a86e9ac`: 12 dos 19 arquivos `.c` pre-processam identicos com
  `-DMUST_MATCH` e sem `MELEE_HOST`; `lb_013B.c`, `lbarchive.c`, `lbfile.c` e
  `lbmemory.c` geram objetos identicos com `cc -m32 -O2 -DMUST_MATCH`;
  `ftdata.c`, `gm_1A3F.c` e `lbdvd.c` diferem so por `HSD_DevComArg` no lugar de
  `int`, que e `int` na build PowerPC, e nao compilam com o `cc` do host para a
  comparacao de objeto.
- [x] Amostras de audio com 32 bits no host. `axfx.h`, `ax.h` e os efeitos
  AXFX declaravam amostras, ponteiros de amostra e contadores de linha de
  atraso como `long`, que tem 64 bits no host. `AXFXDelaySettings`, que o boot
  alcanca por `lbAudioAx_8002838C`, reservava `n * 4` bytes e zerava `n * 8`,
  alem do bloco. Agora sao `s32` e `u32`, que na build PowerPC sao o mesmo
  `signed long` e `unsigned long`. O build de debug passava por cima do erro; o
  ASan parou nele.
- [x] Bloco de `.bss` do `toy.c` num objeto so no host. O codigo de trofeus
  enderecava de `0x804A26B8` a `0x804A2ABC` como um `Toy26B8`, atravessando
  cinco objetos que o DOL guarda em sequencia, e `Toy_80311960`, que o boot
  alcanca por `gmMainLib_8015F600`, escrevia fora deles. Sob `MELEE_HOST` o
  bloco e uma unica definicao de `Toy26B8`, e `_Toy_804A26B8`, os dois buffers
  de DevText, `Toy_804A284C` e `Toy_804A2AA8` sao macros para as partes nos
  deslocamentos do DOL, com o tipo de array preservado. Um `_Static_assert`
  confere o deslocamento `0x3F0` de `Toy_804A2AA8`.
- [x] Matching dessas duas mudancas contra `3b01b4a5f`: `axfx.c`, `delay.c` e
  `AXAux.c` geram objetos identicos com `cc -m32 -O2 -DMUST_MATCH`; em
  `chorus.c`, `reverb_hi.c` e `reverb_std.c`, que tem assembly, o
  pre-processamento so troca `long` por `s32` e realinha uma linha; `toy.c`,
  `tydisplay.c`, `tylist.c`, `tyfigupon.c` e `gmmain_lib.c` pre-processam
  identicos, ignorando linhas vazias. `toy.c` nao compila com o `cc` do host
  para comparar objeto, ja no commit base, por asserts de tamanho de `gmm_x0`.
- [x] Medido depois delas: `host-debug` com 167/167 testes unitarios e ctest
  10/10; `host-sanitize` com ctest 10/10, antes 8/10 com o boot parando no
  ASan. A cena de titulo segue com 9 GObjs (8 com render), 3 cameras, 1 luz,
  1 fog, 2 modelos, 4 processos, 37 JObjs e 2 LObjs.
- [x] Laco de frame original da tela de titulo. `melee-pc --run-title-scene`
  congela o relogio do OS, sobe o boot, entra no titulo e roda `gm_801A4D34`
  com o `on_frame` da cena ate a propria cena pedir para sair. Medido: 621
  frames de jogo (20 de contagem e 601 ate o contador passar de 600), 621
  frames desenhados por `HSD_GObj_80390FC0` e copiados para XFB, 621
  retraces, saida sem botoes, 2679 triangulos capturados no ultimo frame e
  419175000 ticks de OS, exatamente 621 periodos de 1/60 s. Leva 2,0 s em
  `host-debug` e 10,6 s em `host-sanitize`, com o mesmo relatorio.
- [x] O titulo entra pelo estado real de `gmtitlemode.c`: `gm_801A4BD4`,
  `gm_801A4B88` com a `GameSceneInfo` do estado (a `exit_data` que o `onExit`
  le) e `gm_Scene_Title_OnEnter`, o meio de `gm_801A4014`.
- [x] Relogio do OS congelavel (`melee_host_os_time_freeze`): congelado,
  `OSGetTime` e `OSGetTick` so andam por `melee_host_os_time_advance`. A espera
  do jogo, `lb_800195D0`, salta ate o proximo alarme quando o relogio esta
  congelado e a fila bruta de pad esta vazia. `gm_801A4D34` passa por ali uma
  vez antes de rodar os frames enfileirados e outra depois, com a fila vazia,
  entao o salto acontece uma vez por frame.
- [x] Boot com a inicializacao de pad e video de `main()`: `lb_80019AAC` com um
  espelho de `gmMain_8015FD24`, que e `static` (fila de 5 amostras, 12
  entradas de rumble, clamps de stick e gatilho), o callback vazio de
  `gmMain_8015FDA0` como pos-retrace, `HSD_VIDrawDoneXFB` como callback de
  draw done e `HSD_VISetBlack(0)`. O alarme periodico de pad fica armado desde
  o boot.
- [x] `VIWaitForRetrace` entrega a cerca de draw done pendente antes do
  retrace. Pela leitura de `video.c`, sem essa interrupcao o XFB desenhado
  fica em `WAITDONE`, e `HSD_VIWaitXFBDrawEnable` e o `HSD_VIWaitXFBFlush` do
  fim do laco esperam retraces para sempre.
- [x] Frame sink do GX (`melee_host_gx_set_frame_sink`): `GXCopyDisp` entrega a
  captura do frame ao sink e depois a limpa. Sem sink nada muda para os
  diagnosticos de um frame.
- [x] Modulos originais que entraram por esse laco: `gmtitlemode.c`,
  `hsd_392C.c` e `hsd_3933.c` (as bombas de eventos do adaptador USB, que
  retornam na hora: so os callbacks que `MCCInit` e `MCCOpen` registram enchem
  essas filas), `dbscreenshot.c` e `lbcardgame.c`.
- [x] Perifericos ausentes (`port/src/os/absent_devices.c`): as sondagens de
  MCC e FIO respondem adaptador ausente nos termos da SDK (`MCCInit`,
  `FIOInit` e `MCCOpen` devolvem 0, `MCCGetLastError` devolve 1, que o jogo
  chama de "MCC is no initialize"); o que so faz sentido com um canal aberto
  para com o nome. `CARDProbe` nao encontra cartao.
- [x] Paradas com nome em `unported.c`: `gm_80173754` e `gm_80173EEC`, que o
  `onExit` do estado de titulo chama e o host ainda nao roda;
  `HSD_Leak_80387DF8`, `hsd_80398310` e `OSCheckActiveThreads`, que o laco so
  alcanca nos niveis de depuracao. `db_PrintThreadInfo` ganhou um ramo
  `MELEE_HOST`: `_stack_addr` e `_stack_end` sao limites da pilha no linker
  script do DOL, e o host nao tem essa regiao.
- [x] Tres relatos do UBSan no caminho do titulo ficaram explicitos sob
  `MELEE_HOST`: o alarme de pad armava `fn_800195FC(void)` como
  `OSAlarmHandler` (`lb_0195.c` agora arma um handler do tipo certo),
  `parseFloat` deslocava um byte para o bit de sinal de um `int` (`fobj.c`) e
  `HSD_TExpSetReg` tomava o endereco de um membro de `texp` nulo (`texp.c`).
- [x] Matching contra `3b01b4a5f`, com `-DMUST_MATCH` e sem `MELEE_HOST`:
  `lb_0195.c`, `fobj.c` e `texp.c` pre-processam identicos e geram objetos
  identicos com `cc -m32 -O2`; `dbinit.c` pre-processa identico (14213 linhas
  nao vazias) e nao compila com o `cc` do host para comparar objeto, ja no
  commit base.
- [x] Medido ao fim: `host-debug` com 171/171 testes unitarios (novos: relogio
  congelado, fila de alarmes vazia, frame sink e draw done no retrace) e ctest
  11/11; `host-sanitize` com ctest 11/11. Os testes da cena de titulo e do
  laco nao tem relato do UBSan; na suite inteira restam dois, listados nas
  limitacoes.
- [x] Apresentacao pelo SDL/OpenGL do laco de titulo. `melee-pc
  --view-title-scene assets-local` abre uma janela e mostra cada frame que
  `gm_801A4D34` desenha, a 60 Hz de relogio de parede, com o teclado ou o
  primeiro gamepad como PAD 1 (Enter e START, WASD o analogico, Esc sai).
  `--view-title-scene DIR BMP N` desenha escondido, grava o frame N num BMP e
  imprime a captura daquele frame. Medido no frame 120: 2679 triangulos em 33
  runs, 37 texturas e uma unica view (perspectiva, viewport 640x480); a
  imagem mostra o logo, "Melee", "PRESS START" e as tres linhas de copyright.
  O frame 120 escondido leva 1,0 s em `host-debug`.
- [x] A captura do GX guarda, por draw, a projecao nos seis numeros do GX, o
  viewport com near e far e o scissor (`melee_host_gx_captured_view_state_*`,
  indice `view_state` em cada vertice), como ja guardava estado de pixel e TEV.
- [x] `port/src/gx/view.{hpp,cpp}`: projecao GX para o clip do GL, com a
  profundidade remapeada para 2z + w (o GX poe near em -w e far em 0), e
  retangulos do framebuffer (origem no topo) para a janela. Testado com
  matrizes de `MTXPerspective` e `MTXOrtho` montadas a mao.
- [x] `melee::render::FramePresenter`: desenha a captura de um frame na ordem
  do jogo, cada run com o viewport, o depth range e o scissor da sua view,
  sobre a cor e a profundidade de clear da copia de display. O front face do
  GL e horario por essa projecao: com anti-horario o anel de "PRESS START"
  aparecia pelo avesso e o logo sumia.
- [x] Texturas de intensidade (`I4`, `I8`) decodificadas com alfa igual a
  intensidade, como o GX as entrega ao TEV. Com alfa 255 as linhas de
  copyright e o simbolo de marca viravam retangulos brancos e o logo ficava
  escuro. O teste de I4 afirmava o alfa errado e foi corrigido; ha um teste de
  I8.
- [x] `decode_captured_textures` calculava o tamanho de uma textura fora do
  `try`, e um formato desconhecido derrubava o processo em vez de virar um
  texel branco. O titulo tem um: a textura Z8 do apagamento de tela.
- [x] Medido depois dessas mudancas: 176/176 testes unitarios e ctest 11/11 em
  `host-debug` e em `host-sanitize`; os relatos do UBSan continuam os dois do
  sistema de classes do HSD.
- [x] Modo de titulo inteiro pelo codigo do jogo. `melee-pc --run-modes`
  (antes `--run-title-mode`) roda `runGameMode` com o modo de titulo de uma
  tabela de modos e cenas do
  host (`port/src/game/game_tables.c`), no lugar de `gmscdata.c`, que ligaria o
  jogo inteiro: preload do estado, `on_enter`, cena, laco de frame, `onExit` e
  a espera do cartao que fecha todo estado. Medido: sem botoes, 621 frames e o modo seguinte e o filme
  de abertura (`0x18`); com START a partir do frame desenhado 120, 122 frames e
  o modo seguinte e o menu (`0x01`), pelo `onExit` original, com
  `gm_80173EEC`, `gm_80172898` e `gm_80173754` de `gm_1736.c`. Leva 2,0 s e
  0,4 s em `host-debug`, 10,0 s e 2,2 s em `host-sanitize`. Virou o teste
  `melee-host-title-mode-asset`; o caso com START passou ao teste do menu.
- [x] Cartao de memoria ausente. O boot roda `lb_8001C5BC`, `lb_8001D21C` e
  `lbSnap_8001E290`, e as chamadas CARD respondem `CARD_RESULT_NOCARD`
  (`port/src/os/absent_devices.c`). `lbcardnew.c`, `hsd_3A94.c`, `hsd_3B27.c`,
  `hsd_3B2B.c`, `hsd_3B2E.c`, `hsd_4D11.c`, `gm_1736.c` e `tydisplay.c`
  entraram no build. `hsd_3A94.c` le de `0x804D1138` a `0x804D2648` como um
  `CardContext` so, atravessando tres objetos de `.bss`; sob `MELEE_HOST` eles
  sao um bloco unico com as partes nos deslocamentos do DOL (`hsd_4D11.c`),
  como em `toy.c`. `DVDCheckDisk` responde disco presente enquanto ha backend
  e `OSResetSystem` para com nome.
- [x] Preload da demo do titulo. O estado de titulo mantem todos os heaps de
  preload (`lbDvdPreload_3`), e `gm_PreloadTitleDemo` registra os arquivos dos
  lutadores, do estagio e dos efeitos sorteados, que carregam em segundo plano
  pelo devcom enquanto o titulo roda; os dos heaps 4 e 5 vao para a ARAM. Isso
  trouxe `ftdata.c`, `efasync.c` e 34 arquivos de personagem: os 33 que definem
  nome de arquivo, strings e lista de figurino de cada lutador (so Mario e
  Kirby tem arquivo so de dados; nos outros e o arquivo principal, e o
  `--gc-sections` descarta o codigo) e `ftkirby.c`, com a lista de figurinos e
  o preload das habilidades de copia. Referencia `weak` nao serve aqui:
  `ftData_800855C8` desreferencia a tabela de figurinos do lutador. Os 36
  arquivos compilam sem erro no host, e as paradas de `ftData_800855C8`,
  `ftData_8008578C` e `efAsync_OnLoad` sairam de `unported.c`.
- [x] ARQ do host com transferencia de verdade e entrega adiada. A ARAM passou
  a ter conteudo: um buffer de 16 MB indexado pelo offset, que continua sem ser
  ponteiro de processo. `ARQPostRequest` nao completa mais dentro da chamada:
  a copia e o callback acontecem no passo seguinte do escalonador do backend
  (`melee_host_dvd_schedule_backend_task`), na ordem de postagem. O devcom
  depende disso: posta a ultima transferencia de um pedido para ARAM e so
  depois o desliga, e o callback da transferencia devolve o pedido a lista
  livre. Completando dentro da chamada, a fila passava a apontar para a lista
  livre, um pedido ja liberado voltava a rodar e o jogo parava em
  `devcom.c:36`. Dois testes novos: transferencia nos dois sentidos na ordem
  postada, e devcom tipo `0x23` com um segundo pedido na mesma fila.
- [x] `tydisplay.c` dimensionava o vetor de arquivos de trofeu como
  `0xB0 / sizeof(HSD_Archive*)`: 44 entradas no console e 22 no host, enquanto
  `tyDisplay_8031C8B8`, que o preload de todo estado chama, limpa 43. As 21
  escritas a mais caiam no objeto seguinte do `.bss` do host, que era o estado
  do laco de modos do host (`game_tables.c`), e o START do teste nunca chegava
  ao jogo. Sob
  `MELEE_HOST` o vetor tem as 44 entradas. E o unico vetor da arvore
  dimensionado por tamanho de ponteiro.
- [x] Matching das mudancas na decomp e na SDK de `cc0c7ef4e` e desta etapa,
  contra `d3de55f46`, com `-DMUST_MATCH` e sem `MELEE_HOST`: `gm_1A3F.c`,
  `hsd_4D11.c` e `tydisplay.c` pre-processam identicos; em `hsd_3A94.c`,
  `lbcardnew.c` e `lbcardgame.c` as unicas 14 linhas diferentes sao as
  declaracoes CARD com `s32` no lugar de `long`, que na build PowerPC e o mesmo
  `signed long`. A conferencia achou que `cc0c7ef4e` tinha trocado o retorno de
  `lb_8001B8C8` e `lb_8001BA44` de `bool` para `int` fora de `MELEE_HOST`, o
  que muda o que `gm_1AED.c`, `soundtest.c` e `lbcardgame.c` veem; a troca
  voltou a valer so no host.
- [x] Medido ao fim: `host-debug` com 178/178 testes unitarios e ctest 13/13;
  `host-sanitize` com 178/178 e ctest 13/13, com os mesmos dois relatos do
  UBSan do sistema de classes e nenhum relato novo.
- [x] Menu principal pelo codigo do jogo. `melee-pc --run-modes` comeca o
  roteamento em um modo e roda um modo por vez como o laco de `gm_801A4510`
  (`gm_HostBeginGameModes` e `gm_HostRunCurrentGameMode`, sob `MELEE_HOST`, com
  `runGameMode` de volta a `static`), apertando botoes por frame. A tabela do
  host ganhou `GM_MENU` (`gmmenumode.c`) e a cena `GS_MENU`
  (`mnMain_Scene_OnEnter` e `mnMain_Scene_OnFrame`). Medido: titulo, START no
  frame 120, menu, DOWN, A e A escolhem VS Melee; rota `0x00` (122 frames),
  `0x01` (120 frames), `0x02`. Leva 1,1 s em `host-debug` e 6,0 s em
  `host-sanitize`. Virou o teste `melee-host-main-menu-asset`.
- [x] Modulos que o menu alcanca: `gmmenumode.c`, `mngallery.c`, `mnsnap.c`,
  `gmevent.c`, `gmhowto.c`, `gmhomerun.c`, `ft_0C31.c` (a copia fora de linha de
  `HSD_JObjSetMtxDirty`) e `hsd_3B5C.c`, todos sem instrumentacao no
  `host-sanitize`. As telas que o menu abre alem de VS Melee (multi-man, contagem,
  diagrama, informacoes, teste de som, apagar dados, deflicker, idioma, som,
  vibracao, regras, eventos, nomes e selecao de personagem pelo nome) e o
  decodificador THP param com nome em `unported.c`.
- [x] Semantica GNU89 para `inline` nas fontes C do jogo. O MWCC emite fora de
  linha uma definicao `inline` sem `static`, e o C99 nao, o que deixava
  `gmMainLib_AdjustNameTag` e `GetAutoNameCharacter` sem definicao a -O0.
  Nenhum header tem definicao `inline` sem `static`, entao nada sai duplicado.
- [x] SDK: `GXGetTexObjFmt`, `GXGetTexObjWidth` e `GXGetTexObjHeight` leem o
  objeto empacotado; `GXSetTevSwapModeTable` aceita as tabelas do `GXInit`, que
  sao as que o TEV avalia, e para com nome em qualquer outra (a unica chamada, da
  biblioteca de sprites, instala a SWAP0 do `GXInit`); `DCFlushRange` e
  `AXSetVoiceCurrentAddr` como as irmas.
- [x] Tabelas de texto SIS (`SIS_*`) traduzidas: um ponteiro por string, tantos
  quantos campos relocados seguidos. Nos 28 arquivos `Sd*` a tabela fica no
  inicio dos dados e todas as relocacoes do arquivo sao entradas dela; as
  strings ficam verbatim. Quatro arquivos apontam a primeira entrada para o fim
  dos dados, onde o console leria a tabela de relocacao, cujo primeiro campo e
  zero, e o host da a ela quatro bytes zero.
- [x] Interpretador de texto SIS portado sob `MELEE_HOST` (`hsd_3A76.c`): leitura
  big-endian de glifos, escala, espacamento, posicao e atrasos, que o original
  lia como palavras nativas; o kerning soma o par direto, em vez de passar o
  endereco por `s32`; ponteiro comparado sem truncar; float para `u8` pelo
  `s32`, que o UBSan acusava. Os opcodes 8 e 9, que guardam um endereco de 32
  bits no stream, param com nome, e nenhum arquivo do disco os usa.
  `HSD_SisLib_803A6754` aloca o bloco com `sizeof`, e nao com 16 bytes.
- [x] Tradutores em C para dados que so headers C descrevem
  (`melee_host_hsd_register_translator` e o leitor `melee_host_hsd_reader_*`):
  `sqEventInitDataLevelTbl` (51 niveis de evento, com `evinit`, bonus, estagios
  e jogadores em layout host, e os bit-fields do `evinit` desempacotados do MSB),
  `lbAudioLoadData` (4 tabelas de 30 listas de bancos de som ate `0x83D60`,
  convertidas para a ordem do host) e `MemCardIconData` e `MemSnapIconData`
  (enderecos das imagens do cartao, verbatim). Os seis tipos de evento sairam de
  `gmevent.c` para `gmevent.h`, sem mudar o codigo gerado.
- [x] `VA_END_PTR` nas oito listas variadicas de ponteiros que este caminho roda:
  `mnmain.c`, `gmevent.c` tres vezes, `lbaudio_ax.c`, `lbcardgame.c` duas vezes e
  `lbsnap.c`. A do menu passava no build de debug e escrevia em
  `0x55ae00000000` sob ASan.
- [x] Musica: o inicio do stream (`HSD_Synth_8038B5AC`) usava a voz de
  `AXAcquireVoice` sem conferir, e o host nao entrega voz. Sob `MELEE_HOST`, sem
  voz o stream nao comeca: a flag de ocupado e limpa e volta -1, como quando a
  cadeia do stream nao acha no.
- [x] Os enderecos das imagens do cartao ocupam `intptr_t` onde o jogo os guarda
  em `int` antes de chegar ao cartao: `x5C` em `lbcardgame.static.h`, o
  resultado de `lb_8001C820` e a union de `lbsnap.c`.
- [x] Matching das mudancas desta etapa, contra `d3de55f46`, com `-DMUST_MATCH`
  e sem `MELEE_HOST`: `synth.c`, `hsd_3A76.c`, `hsd_3A64.c` e `gm_1A3F.c`
  pre-processam identicos; `gmevent.c` tem as mesmas linhas, com o bloco de
  tipos movido; token a token, `lbaudio_ax.c` e identico e `mnmain.c`,
  `lbcardgame.c` e `lbsnap.c` so diferem pelos tipos de evento que chegam por
  `gmevent.h` e pelas declaracoes CARD com `s32`.
- [x] Medido ao fim: `host-debug` com 180/180 testes unitarios e ctest 13/13;
  `host-sanitize` com 180/180 e ctest 13/13, e so os dois relatos do UBSan do
  sistema de classes.
- [x] Sincronizado com `doldecomp/melee` ate `d9f54dbc0` (11 commits) por
  merge, que preserva os hashes ja publicados em `origin`. Conflitos em `os.h` e
  `debug.h`, onde a upstream passou `OSPanic`, `__assert` e `HSD_Panic` a
  `const char*` e as definicoes do host acompanharam, e em `gmevent.c`, onde a
  upstream separou `struct gm_804D6900_x4_t`, que agora vive em `gmevent.h`.
  `seed_ptr` virou `HSD_RandSeedPtr`, e o teste do gerador acompanhou.
- [x] A conferencia contra a upstream achou `OSReport(const char*)` declarado
  tambem para a build PowerPC desde `4c4ecadef`, enquanto `OSError.c` o define
  com `char*`. O `const` ficou so sob `MELEE_HOST`. Token a token contra
  `d9f54dbc0`, com `HSD_DevComArg`, `OSRtcUlong` e `ARQAddress` normalizados
  para os tipos PowerPC, `gmevent.c`, `lbaudio_ax.c`, `gmscene.c`,
  `gmmain_lib.c` e `hsd_3A76.c` so diferem pelo que o port ja documenta, e em
  `synth.c` sobra um bloco de declaracoes antecipadas em outra posicao, que so
  difere pelo mesmo `HSD_DevComArg`. Medido depois: 180/180 e ctest 13/13 em
  `host-debug` e em `host-sanitize`, com os dois relatos do UBSan de antes.
- [x] Presets de debug/sanitizers e workflow multiplataforma.
- [x] Selecao de personagens (`GS_CSS`) e de estagio (`GS_SSS`) executadas pelo
  codigo do jogo dentro do modo `GM_VS`, com entrada de dois pads. O roteiro de
  `melee-pc --run-modes` abre as portas 1 e 2 pelo botao HMN, leva as duas
  fichas ate a Fox, aperta START e escolhe Hyrule Temple na SSS. Medido:
  titulo 122 frames, menu 120, CSS 141 e SSS 149; a selecao lida de volta do
  modo VS e o estagio 14 com Fox (2) nos slots 0 e 1, e o modo para na cena de
  luta (`GS_VS`, 0x02), que a tabela do host ainda nao tem. Leva 4,2 s em
  `host-debug`. Virou o teste `melee-host-vs-selection-asset`.
- [x] Pool de texto SIS dimensionado para o host. `preloadState` cria o pool a
  cada estado com tamanhos pensados para o PowerPC (0x2400 bytes para a CSS,
  0xC000 para creditos e resultados, 0x4800 para o resto), e o alocador poe um
  cabecalho `SisBlock` antes de cada bloco. No host o cabecalho tem 24 bytes em
  vez de 12 e um `HSD_Text` tem 192 em vez de 160, e a CSS esgotava o pool ao
  montar o texto das portas: panic em `sislib.c:95` ("Memory Empty") com 33
  blocos usados e 896 bytes livres. Sob `MELEE_HOST` o pool tem o dobro do
  tamanho pedido e os blocos arredondam para o alinhamento de ponteiro, porque
  o cabecalho do bloco seguinte comeca onde os dados terminam; assim nenhum
  bloco custa mais que o dobro do que custa no console.
- [x] Cena ausente para com nome. `gm_801A4014` chama pelo que
  `gm_FindGameSceneHandler` devolve, que e NULL para uma cena fora da tabela.
  Sob `MELEE_HOST` o estado cuja cena falta encerra o modo antes do preload
  (`gm_HostMissingScene`), e o relatorio do modo diz qual cena foi.
- [x] Relatorio por cena: `MeleeHostGameModeReport` lista a cena de cada estado
  que o modo rodou, com os frames desenhados nela (`gm_HostSceneEntered`,
  implementado pela tabela do host), e `--run-modes` imprime uma linha por cena
  e o resumo `scenes:`.
- [x] Roteiro de entrada com stick e quatro portas:
  `FRAME[-ULTIMO]:ENTRADA[+ENTRADA][@PORTA]`, onde a entrada e um botao ou
  `SX=N`/`SY=N` para o stick principal. Uma porta citada no roteiro fica
  conectada desde o inicio. Sem `-ULTIMO` o aperto dura tres frames, como
  antes, e os testes de titulo e menu continuam com a mesma linha `route:`.
- [x] `melee_host_vs_selection_get`: a mesma observacao segura de
  `MeleeHostPreparedMatch`, lida do `VsModeData` que a CSS e a SSS escrevem,
  sem expor layout PPC ao C++. Le o campo de `gmMainLib_804D3EE0` que
  `gmVsMelee_GetVsData` devolve, em vez de chamar a funcao, para que um
  binario que so observa nao puxe `gmvsmelee.c` e a maquina de estados VS.
- [x] Build `host-sanitize` de volta. Desde `f1cf24150`, que pos `gmvsmelee.c`
  no build, `melee-pc` e `melee-host-tests` nao ligavam sob sanitizers:
  `gmVsMelee_EnterResults` chama `gm_80177724`, de `gmresultplayer.c`, que nao
  esta no build. O `--gc-sections` descarta o chamador no `host-debug`, e a
  build instrumentada o mantem. Como o modo VS do host termina na luta e nunca
  chega aos resultados, `gm_80177724` para com nome em `unported.c`, e
  `gmvsmelee.c`, que a rota executa, continua instrumentado.
- [x] Tabela de estagios da SSS lida alem do fim, como no console.
  `fn_8025A090` le `x8` e `x9` de `mnStageSel_803F06D0[30]` quando o cursor
  esta no estagio aleatorio, uma entrada depois das 30 que a tabela tem
  (`symbols.txt` da 0x348 bytes). No console isso cai na string
  `"MnSlMap.usd"`, que vem logo depois no `.data`: `x8` e `'u'` (117) e `x9` e
  `'s'` (115), lidos do `main.dol` extraido. No host a leitura saia do objeto;
  o build de debug seguia com o que houvesse ali, e o ASan abortava a rota da
  SSS com `global-buffer-overflow`. Sob `MELEE_HOST` a tabela tem uma 31a
  entrada com esses bytes, e sem o define continua com 30.
- [x] Shape animation le os arrays de vertice em big-endian. Os payloads GX
  ficam com os bytes do disco, porque o interpretador de display list os le
  nessa ordem, mas `drawShapeAnim` mistura posicoes, normais e NBT na CPU
  (`get_shape_vertex_xyz`, `get_shape_normal_xyz`, `get_shape_nbt_xyz`) com
  `memcpy` para `f32` e leituras `*(u16*)`/`*(s16*)`. No host isso dava lixo: o
  painel da SSS chegava ao GX com posicao (-2,1e-38; 2,05; NaN), e o UBSan
  acusava a conversao do NaN para `u8` na iluminacao do host. Sob `MELEE_HOST`
  os componentes de 16 e 32 bits sao montados dos bytes; o mesmo painel sai com
  (45,22; 35,25; -9,998) e normal (0; 0; 1). O interpretador da display list de
  shape animation ja montava os indices byte a byte e nao mudou. Todo modelo
  com shape animation passava por aqui.
- [x] Matching desta etapa contra `f1cf24150`, com `-DMUST_MATCH` e sem
  `MELEE_HOST`: `sislib.c` (4.111 linhas nao vazias), `gm_1A3F.c` (10.349),
  `pobj.c` (5.509) e `mnstagesel.c` com seu `.static.h` (11.449) pre-processam
  identicos;
  `gm_1A3F.h` so ganhou declaracoes sob `MELEE_HOST`. As mudancas de
  `f1cf24150` na decomp tambem foram conferidas contra o commit anterior a
  ele: `mncharsel.c`, `mnmain.c`, `gmmenumode.c` e `gmvsmode.c` pre-processam
  identicos.
- [x] Medido ao fim: `host-debug` com 180/180 testes unitarios e ctest 14/14, a
  rota VS em 4,2 s; `host-sanitize` com 180/180 e ctest 14/14, a rota VS em
  20,4 s, nenhum erro do ASan e, do UBSan, so relatos de chamada por ponteiro de
  funcao de outro tipo (os dois de antes e dois que a CSS e a SSS alcancam).
- [x] `host-sanitize` sem arquivos excluidos da instrumentacao. Os 30 arquivos
  da decomp que compilavam sem sanitizers (jogadores, `ground.c`,
  `lbarchive.c`, menus, `ftdata.c` e os arquivos de lutador, `efasync.c` e
  outros) estavam fora porque os metadados de globais do ASan mantinham vivo,
  apesar do `--gc-sections`, o grafo de modulos que eles citam e o host nao
  compila. Duas opcoes resolvem: `-fsanitize-address-globals-dead-stripping`
  poe a descricao de cada global numa secao ligada a do proprio global, e
  `-Wl,-z,start-stop-gc` impede o GNU ld de manter toda secao que o runtime do
  ASan encontra por `__start_asan_globals`. Com as duas, o link sanitizado
  descarta o mesmo que o de debug, e todo modulo compilado e instrumentado.
- [x] O que a instrumentacao desses arquivos encontrou, corrigido sob
  `MELEE_HOST`:
  - `fn_8022AFEC` (`mnmain.c`) guardava um JObj por opcao do menu num vetor de
    4 na pilha; o menu principal tem 5 opcoes e o de opcoes 10. O ASan parava
    as rotas do menu e da selecao VS com `stack-buffer-overflow`. No host o
    vetor tem 10 entradas.
  - `gmMainLib_8015F4E8` le a vibracao da porta 5 de um vetor de 4. No console
    isso cai no byte `deflicker` de `GamePrefs` (+0x15), e o host o le pelo
    nome.
  - O bit 31 das palavras de flags de `gmmain_lib.c` deslocava 1 para o bit
    de sinal de um `int`; o host desloca `1U`.
  - `efAsync_LoadAsync` e `efAsync_LoadSync` tomavam o endereco da entrada
    antes de conferir o indice, e sao chamadas com 255; o host confere antes.
- [x] Matching dessas mudancas contra `1a36cb89d`, com `-DMUST_MATCH` e sem
  `MELEE_HOST`: `mnmain.c` (13.710 linhas nao vazias), `gmmain_lib.c`
  (10.887) e `efasync.c` (8.169) pre-processam identicos. Em `gmmain_lib.c` a
  macro `GMMAINLIB_FLAG` expande, sem o define, para os mesmos tokens de antes.
- [x] Medido: `host-debug` com 180/180 e ctest 14/14; `host-sanitize`, agora
  sem excecoes, com 180/180, ctest 14/14, rota VS em 20,0 s, nenhum erro do
  ASan e, do UBSan, os mesmos quatro relatos de chamada por ponteiro de funcao.
- [x] Sondagem de compilacao do resto da decomp: dos 808 `.c` de `src/melee`
  que o core nao lista, 741 passam em `-fsyntax-only` com as flags do core.
  Das 67 falhas, 59 sao estagios cujo `on_demo_init` recebe `bool` onde
  `StageData` declara `int`; as outras sao pontuais: escrita direta em
  `GXWGFifo` (`gm_1832.c`), declaracoes que chegam ao PowerPC por outro caminho
  (`OS_TIMER_CLOCK`, `OSSetProgressiveMode`, `OSPanic`), um callback de item
  e tipos de callback em `grpstadium.c` e `grshrineroute.c`.
- [x] O resto da decomp no core. Todo `.c` de `src/melee` entra no
  `melee_game_core` por um glob no `port/CMakeLists.txt`, menos `gmscdata.c`
  (as tabelas de modos e cenas do host o substituem), junto de `particle.c`,
  `generator.c`, `psappsrt.c` e `quatlib.c` do baselib e das constantes de
  `src/MSL/float.c`. Como os dois presets descartam o que nada alcanca, o
  executavel so ganha o que as cenas chamam: as rotas de titulo, menu e
  selecao VS continuam com os mesmos frames. Ficam fora `archive.c` (substituido
  por `hsd_host_archive.cpp`), `debug.c` (E/S da MSL; o host implementa
  `OSPanic` e `HSD_Panic`), `psdisp.c` (escreve direto em `GXWGFifo`) e
  `sislib_font.c` (o `.inc` gerado do DOL).
- [x] O que precisou mudar para os 808 arquivos compilarem, sob `MELEE_HOST`:
  - `StageData::on_demo_init` recebe `int`, e `Stage_8022532C` passa `0x19` e
    `0x1A` nas demos de Final Destination, que `grLast_OnDemoInit` compara.
    Cerca de 60 estagios declaravam o parametro como `bool`, que no host e
    outro tipo de funcao e dobraria 26 em 1. A macro `GrDemoInitArg`
    (`gr/forward.h`) e `int` no host e `bool` fora dele.
  - Tipos que a declaracao e a definicao discordavam: `grLast_8021B5C4`
    devolve 0 a 3 e estava declarado `bool`; `grLib_801C9EE8`,
    `grShrineRoute_8020B020`, `itMewtwodisable_UnkMotion0_Coll` e os callbacks
    de rota de `gricemt.c` e `grkinokoroute.h` passam a concordar com quem os
    chama.
  - `gm_1832.c` escrevia posicoes direto em `GXWGFifo`; o host usa
    `GXPosition3f32`, como em `hsd_3915.c`. `gm_1884.c`, `gmprogressive.c`,
    `gmregclear.c` e `grpstadium.c` incluem `dolphin/os.h`, que a build PowerPC
    recebe por outro caminho. O callback de DevCom de `grpstadium.c` recebe
    `HSD_DevComArg`.
- [x] Novas pecas do host para o que passou a ser alcancado: `GXProject` (a
  aritmetica da SDK, na mesma ordem de operacoes) e `GXSetTevClampMode` (a
  SDK so avisa que o hardware nao tem esse modo) em
  `port/src/gx/state_recorder.cpp`.
- [x] Paradas de `unported.c` que viraram codigo original: `gm_80177724`,
  `grDatFiles_801C5FC0` e `psInitDataBankLocate`. As duas funcoes de
  `particle.c` que relocam bancos de particula no lugar com enderecos de 32
  bits, `psInitDataBankLocate` e `psInitDataBankLoad`, param com nome dentro
  do proprio arquivo sob `MELEE_HOST`.
- [x] Matching contra `1f493384a`, token a token, com `-DMUST_MATCH` e sem
  `MELEE_HOST`: os 77 `gr/*.c`, `gm_1832.c`, `gm_1884.c`, `gmprogressive.c`,
  `gmregclear.c`, `itmewtwodisable.c` e `particle.c` sao identicos, menos
  `grpstadium.c`, que so difere por `HSD_DevComArg` no lugar de `int`, o mesmo
  tipo na build PowerPC.
- [x] Medido: `host-debug` com 180/180 e ctest 14/14; o `melee-pc` de debug
  tem 64 MB. `host-sanitize` com 180/180, ctest 14/14, rota VS em 19,9 s,
  nenhum erro do ASan e, do UBSan, so os quatro relatos de chamada por ponteiro
  de funcao de antes.
- [x] Trigonometria do proprio jogo. `atan2f`, `acosf` e `asinf` passaram a vir
  de `lbtrigf.c`; antes dele entrar no core, o cursor da CSS ligava o `atan2f`
  da glibc. As rotas seguem com os mesmos frames. `SIGN_BIT` desloca `1U` sob
  `MELEE_HOST`, porque o UBSan acusava `1 << 31` em `int`, e `lbtrigf.c`
  pre-processa identico sem o define. Nenhum simbolo C e definido ao mesmo
  tempo no core e em `libmelee_host.a`, entao nenhum modulo da decomp esconde
  uma implementacao do host, nem o contrario.
- [x] A cena de luta liga. Com `psdisp.c` e `psdisptev.c` no core e tres pecas
  novas no GX do host, `GS_VS` na tabela deixa o `melee-pc` sem nenhum simbolo
  indefinido (67 MB em debug):
  - `psdisp.c` escrevia os vertices das particulas direto em `GXWGFifo` (35
    escritas de `f32` e de `u8`). Sob `MELEE_HOST` as macros `PS_FIFO_F32` e
    `PS_FIFO_U8` entregam os mesmos bytes, na mesma ordem e largura, a
    `melee_host_gx_submit_f32` e `_u8`, que o recorder le pelo descritor de
    vertice corrente. Sem o define elas expandem para a escrita original, e
    `psdisp.c` pre-processa para os mesmos tokens.
  - Textura indireta registrada no recorder (`GXSetNumIndStages`,
    `GXSetIndTexOrder`, `GXSetIndTexCoordScale`, `GXSetIndTexMtx`,
    `GXSetTevIndirect` e `GXSetTevDirect`), que `lbrefract.c` usa, e
    `GXEnableTexOffsets`, que `psdisp.c` usa. O TEV por fragmento e o presenter
    ainda nao avaliam nenhum dos dois.
- [x] Primeiro passo da luta em execucao, medido com `GS_VS` posto na tabela so
  localmente: depois da SSS o modo VS monta o estado da luta,
  `gm_Scene_Vs_OnEnter` passa por `db_Setup` e pela inicializacao da camera e
  para em `lbRefract_800222A4`, que pede `lbRefData` de `LbRf.dat`, um simbolo
  que a API de arquivo do host ainda nao traduz (assert de `lbarchive.c:87`).
  A entrada nao foi commitada, porque o teste da selecao VS espera a parada
  com nome na cena de luta.
- [x] Medido sem a entrada: `host-debug` com 180/180 e ctest 14/14;
  `host-sanitize` com 180/180, ctest 14/14, rota VS em 19,8 s, nenhum erro do
  ASan e, do UBSan, os mesmos quatro relatos de chamada por ponteiro de funcao.
- [x] `lbRefData` traduzido (`port/src/game/game_data_translators.c`). O
  `LbRf.dat` tem 32 bytes de dados: seis floats e, em `data+0x18`, um registro
  com a contagem (3) e um ponteiro relocado para eles, dois floats por tipo de
  refracao. `lbrefract.c` declara o registro em particular como `u8` e
  ponteiro; o tradutor monta o mesmo layout com os floats convertidos de
  big-endian e recusa a tabela sem ponteiro. Um teste unitario monta o
  arquivo com o `ArchiveBuilder` e confere contagem, floats e a recusa.
- [x] Com `GS_VS` na tabela so localmente, a entrada da luta passa pela
  refracao, `lb_8000FCDC` e `efLib_Init`, e para em `efAsync_LoadSync(0)`:
  `EfCoData.dat` (1,3 MB, 4.321 relocacoes, um simbolo publico) pede
  `effCommonDataTable`, que o host nao traduz, e `efAsync_OnLoad` segue o NULL
  que recebe. A tabela aponta os bancos de comando e de textura das
  particulas, que `psInitDataBankLocate` e `psInitDataBankLoad` relocam no
  lugar com enderecos de 32 bits.
- [x] Os diagnosticos `--load-archive` e `--sweep-archives` nao fazem o boot e
  por isso nao registravam os tradutores C do jogo: la um simbolo como
  `lbRefData` aparecia como sem traducao, embora a rota o traduza. Desde
  14/09/2026 eles os registram (ver adiante).
- [x] Medido: `host-debug` com 181/181 e ctest 14/14; `host-sanitize` com
  181/181, ctest 14/14, rota VS em 20,0 s, nenhum erro do ASan e, do UBSan, os
  mesmos quatro relatos de chamada por ponteiro de funcao.
- [x] Loader de particulas do host. No console `psInitDataBankLocate` reescreve
  no lugar os offsets de 32 bits dos bancos em enderecos de 32 bits, e
  `psInitDataBankLoad` monta tabelas de ponteiros para dentro deles. No host a
  API de arquivo monta os bancos ja localizados, em largura de ponteiro
  (`port/src/assets/hsd_materialize.cpp`):
  - `eff*DataTable`, o nome que `efAsync_DatEntries` da a tabela de cada
    arquivo de efeito, vira `MaterializedEffectTable`: os dois bancos e o vetor
    de `EF_EffectDesc` no layout do host, que `efAsync_LoadSync` enxerga como
    `EF_DAT_Entry` e cujo `data` e o primeiro efeito. A tabela nao grava
    quantos efeitos tem; o vetor vai de `+0x8` em registros de 0x14 bytes ate
    o primeiro endereco que a tabela, os bancos ou um registro anterior
    apontam, que em todo arquivo do disco vem depois da tabela.
  - O banco de comandos vira `MeleeHostParticleCmdBank` (`psstructs.h`): o
    cabecalho de cada `HSD_PSCmdList` convertido, `kind` com os bits que a
    segunda fase de `psInitDataBankLocate` poe, e `cmdList`, que no console e o
    vetor embutido em `+0x3C`, como ponteiro para os bytes de comando
    verbatim, que o interpretador le byte a byte em big-endian. A tabela e
    indexada pelo ID da lista, e os IDs de um banco comecam no primeiro que ele
    grava; por isso ela fica fora da arena de descritores.
  - O banco de texturas vira `MeleeHostParticleTexBank`: os escalares de cada
    `HSD_PSTexGroup` convertidos e a tabela de imagens e paletas como
    enderecos no payload verbatim. Uma entrada que o banco nao cobre fica
    NULL: o primeiro grupo de `EfKbSs.dat` e C8 sem contagem nem flag de
    paleta, e a palavra que seria a paleta vale `0x80A8812A`.
  - `map_ptcl` e `map_texg`, os bancos proprios de um estagio que
    `grDatFiles` passa ao sistema de particulas, sao os mesmos dois bancos.
- [x] Em `particle.c` sob `MELEE_HOST`: `psInitDataBankLocate` so confere a
  assinatura dos bancos; `psInitDataBankLoad` toma deles as tabelas de
  `psCmdListArray`, `ptclref_804D0E5C` e `psTexGroupArray` (a contagem de
  texturas que o console guarda em meio ponteiro de `psFormGroupArray` nunca e
  lida, e nao e guardada); bancos de formas e tabela de referencia param com
  nome. `psReadFloat` monta o float na ordem do host. `hsd_80398F0C` recebia a
  lista de comandos e o gerador como `s32`; no host `PS_POINTER_ARG` e
  `intptr_t`. `efAsync_OnLoad` e `efAsync_LoadSync` testam os dois ponteiros
  inteiros, e nao a metade baixa deles.
- [x] Joints de spline traduzidos (`HSD_Spline`: tipo, pontos de controle na
  contagem que `spline.c` le para cada tipo, comprimentos e polinomios). As
  duas tabelas de luz de `TyLight.dat` que paravam no spline agora param um
  passo depois, com nome: a animacao dessas luzes segue um joint pela chave da
  tabela de IDs, que o host nao resolve.
- [x] Conferido no disco: `--load-archive` de cada uma das 36 tabelas de
  efeito e dos 20 pares `map_ptcl`/`map_texg` traduz os 76 simbolos. A
  varredura segue com 861 arquivos, `game_data` 86/86 e as mesmas duas recusas
  de `TyLight.dat`. Um teste unitario monta tabela, bancos e dois efeitos com o
  `ArchiveBuilder` e confere o cabecalho, o `kind`, os bytes de comando, o
  grupo de textura, o que `psInitDataBank` instala e a recusa de uma tabela com
  um banco so.
- [x] Matching contra `58f18f6a8`, token a token, com `-DMUST_MATCH` e sem
  `MELEE_HOST`: `efasync.c`, `generator.c`, `particle.c`, `psdisp.c` e
  `psappsrt.c` pre-processam identicos (`particle.h` e `psstructs.h` entram
  por eles).
- [x] Com `GS_VS` na tabela so localmente, a entrada da luta passa por
  `efAsync_LoadSync(0)` e `(0x1F)` e para em `Player_80036DD8`, que pede
  `plLoadCommonData` a `PdPm.dat` (assert de `lbarchive.c:87`).
- [x] Medido sem a entrada: `host-debug` com 182/182 e ctest 14/14;
  `host-sanitize` com 182/182, ctest 14/14, rota VS em 19,8 s, nenhum erro do
  ASan e, do UBSan, os mesmos quatro relatos de chamada por ponteiro de funcao.
- [x] `plLoadCommonData` traduzido (`port/src/game/game_data_translators.c`).
  `PdPm.dat` tem 0x188 bytes de dados: a tabela `pl_804D6470_t` em `data+0`
  (0x184 bytes de limiares que `plbonus.c`, `pltrick.c`, `pl_040D.c` e
  `gm_16F1.c` comparam com as estatisticas da partida) e, em `data+0x184`, o
  ponteiro relocado para ela, que e o simbolo; `Player_80036DD8` o
  derreferencia em `pl_804D6470`. Todo campo da tabela e escalar de 4 bytes,
  entao os offsets sao os mesmos no host (um `_Static_assert` confere 0x184) e
  cada palavra e convertida no lugar. `xC0`, que a decomp tipa como quatro
  bytes e ninguem le, fica na ordem do disco. Um teste unitario confere floats,
  inteiros, os bytes e a recusa sem ponteiro.
- [x] Com `GS_VS` na tabela so localmente, a entrada da luta passa pelos dados
  de jogador, `ftCo_800C06C0`, `mpColl_80041C78` e `Ground_801C0378`, e cai
  com SIGSEGV em `Ground_801C0754` (`ground.c:463`): `stage_datas` tem NULL
  para Hyrule Temple. `ground.c` e compilado com `host_weak_stages.h`, que
  declara todo `StageData` como referencia fraca, e uma referencia fraca nao
  puxa `grshrine.c` da biblioteca estatica, embora ele esteja no core desde
  que toda a decomp compila.
- [x] Medido sem a entrada: `host-debug` com 183/183 e ctest 14/14;
  `host-sanitize` com 183/183, ctest 14/14, rota VS em 19,7 s, nenhum erro do
  ASan e, do UBSan, os mesmos quatro relatos de chamada por ponteiro de funcao.
- [x] A tabela de estagios liga forte. `host_weak_stages.h` declarava `weak`
  todo `StageData` que `ground.c` cita, de quando o host nao compilava os
  estagios; com a decomp inteira no core, ele so escondia os estagios da
  biblioteca estatica. Sem ele, `stage_datas` puxa todos os estagios da tabela
  (e `grIzumi_801CD2D4` e `grStadium_801D511C`) e o `melee-pc` liga sem simbolo
  indefinido: 67.258.080 bytes em debug sem a entrada `GS_VS`, 69.969.008 com
  ela. As rotas seguem com os mesmos frames.
- [x] Com `GS_VS` na tabela so localmente, `Ground_801C0754` acha Hyrule
  Temple e `grDatFiles_801C6038` le `GrSh.dat` (1,2 MB, 2.158 relocacoes, 63
  simbolos publicos). Nenhum dos oito simbolos de dados de estagio que ela pede
  tem traducao: `map_head` falta com "Cannot find symbol", os outros voltam
  NULL, e `Ground_801C28CC` cai com SIGSEGV ao seguir `stage_info.param`
  (`ground.c:1509`).
- [x] Medido sem a entrada: `host-debug` com 183/183 e ctest 14/14;
  `host-sanitize` com 183/183, ctest 14/14, rota VS em 19,7 s, nenhum erro do
  ASan e, do UBSan, os mesmos quatro relatos.
- [x] `grGroundParam` traduzido (`port/src/game/game_data_translators.c`). O
  registro de 0xDC bytes tem um unico ponteiro, em `+0xB0`, para as linhas
  `StageParam` (0x64 bytes cada, sem ponteiro), seguido da contagem e de nove
  cores; no host os campos ate o ponteiro mantem o offset e os seguintes andam
  quatro bytes. O tradutor le cada campo no offset do PowerPC, com
  `_Static_assert` do offset do ponteiro e do layout de `StageParam`, e recusa
  linhas contadas sem ponteiro. No `GrSh.dat` as 18 linhas terminam onde o
  registro comeca. Um teste unitario confere escalares, `bool`, cores e duas
  linhas.
- [x] `--load-archive` e `--sweep-archives` registram os tradutores C do jogo
  antes de ler, como o boot faz. Os 71 `grGroundParam` do disco traduzem. A
  varredura passa a `game_data` 170/170 (eram 86) e `unsupported` 5.312, com
  as mesmas duas recusas de `TyLight.dat`.
- [x] Com `GS_VS` na tabela so localmente, a entrada passa por
  `Ground_801C28CC` e para em `Toy_803124BC`, chamado por `Ground_801C5878` e
  `tyDisplay_8031C2CC`: `TyDatai.usd` (19 KB, sem relocacoes, 7 simbolos) nao
  tem traducao para `tyInitModelTbl` (assert de `lbarchive.c:87`).
- [x] Medido sem a entrada: `host-debug` com 184/184 e ctest 14/14;
  `host-sanitize` com 184/184, ctest 14/14, rota VS em 20,0 s, nenhum erro do
  ASan e, do UBSan, os mesmos quatro relatos.
- [x] As sete tabelas de trofeu traduzidas
  (`port/src/game/game_data_translators.c`): `tyInitModelTbl` e
  `tyInitModelDTbl` (`TrophyData`, 0x24 bytes), `tyModelSortTbl`
  (`ToyNameData`, 0xC), `tyExpDifferentTbl` e `tyNoGetUsTbl` (`s16`),
  `tyDisplayModelTbl` e `tyDisplayModelUsTbl` (`TyDspEntry`, 0x10). Nenhuma
  tem ponteiro, entao cada uma mantem o layout do PowerPC, e nenhuma grava o
  proprio tamanho: todas terminam numa linha cujo primeiro campo e -1, que o
  jogo percorre e, quando uma busca nao acha nada, le. O tradutor conta ate
  essa linha, copia-a inteira e, nas tabelas que `toy.c` tambem indexa por
  trofeu (`tyInitModelTbl` e `tyModelSortTbl`), exige `TY_TROPHY_COUNT` (293)
  linhas antes dela; no disco as duas tem exatamente 293. Um teste unitario
  confere as quatro formas e a recusa de uma tabela de ordenacao curta.
- [x] Na varredura as tabelas traduzem nos dois arquivos que as trazem,
  `TyDatai.usd` e `TyDatai.dat` (mesmo tamanho): `game_data` 184/184 e
  `unsupported` 5.298, com as mesmas duas recusas de `TyLight.dat`.
- [x] Com `GS_VS` na tabela so localmente, a entrada do estagio termina:
  `Stage_802251E8` retorna, e `fn_8016E730` para dois passos adiante, em
  `Item_80266F70`, que chama `it_8027870C`. `ItCo.usd` (2,8 MB, 44.204
  relocacoes, um simbolo publico e seis externos, animacoes de modelo de item)
  nao tem traducao para `itPublicData` (assert de `lbarchive.c:87`). O
  registro aponta `ItemCommonData`, tres tabelas de `Article` (atributos,
  atributos proprios de cada item em `void*`, hurtboxes, estados, modelo e
  dinamica), um `it_804D6D40_t` e um `Fighter_804D653C_t`.
- [x] Medido sem a entrada: `host-debug` com 185/185 e ctest 14/14;
  `host-sanitize` com 185/185, ctest 14/14, rota VS em 19,9 s, nenhum erro do
  ASan e, do UBSan, os mesmos quatro relatos.
- [x] Scripts de comando no host. O console le os scripts de lutador, de item
  e de sobreposicao de cor por bit-fields sobre palavras big-endian, que o
  MWCC aloca a partir do bit mais significativo, e por casts de `u8`, `u16` e
  `s16` sobre as mesmas palavras:
  - A API de arquivo converte as palavras de um script no lugar, na copia host
    do payload, para a ordem nativa (`melee_host_hsd_reader_command_stream`):
    uma palavra vale o mesmo inteiro nos dois. Um script nao grava o proprio
    tamanho; a conversao vai ate o primeiro endereco que uma relocacao aponta
    ou uma raiz publica nomeia. Uma palavra relocada so e aceita logo depois de
    uma sub-rotina (5) ou de um goto (7) e vira a distancia ate o alvo, cujo
    script e convertido em seguida; qualquer outro ponteiro e recusado com
    nome. Um mapa das palavras ja convertidas impede converter duas vezes.
  - `port/tools/gen_host_command_layout.py` le as structs de comando e a union
    `ColorOverlay_x8_t` de `lb/types.h`, posiciona cada campo como o MWCC e
    gera `port/src/game/host_command_layout.h`, com os campos de cada palavra
    em ordem inversa e preenchimento onde o console deixa bits sem uso.
    `lb/types.h` inclui o header sob `MELEE_HOST` e mantem as declaracoes
    originais no `#else`. `Command_05` e `Command_07` guardam `rel`, a
    distancia, e a `CmdUnion` segue com 4 bytes. O gerador tambem escreve
    `port/tests/command_layout_check.c`, que empacota valores nos bits do
    modelo do console e os le pelas declaracoes do host: os 255 campos batem.
    O ctest `melee-host-command-layout-generated` falha quando os dois
    arquivos deixam de corresponder a `lb/types.h`. O `scalar_storage_order`
    do GCC evitaria a conversao, mas o clang do `host-sanitize` o ignora.
  - As leituras por cast passam por `CMD_U8`, `CMD_U16` e `CMD_S16`
    (`lb/inlines.h`), que no console expandem para o mesmo cast e no host
    espelham o indice dentro da palavra: 27 em `itanimlist.c` e 1 em
    `ftaction.c`. `itAnimlistCmdUnk`, local de `itanimlist.c`, tem layout host
    proprio.
  - `lbcommand.c` sob `MELEE_HOST`: `Command_03` guarda a contagem num slot de
    ponteiro, `Command_04` conta e volta pelos campos em vez da visao `u32` de
    `CommandInfo`, e `Command_05` e `Command_07` somam a distancia.
  - Um teste unitario monta um script com laco, sub-rotina e goto no formato do
    console, confere as palavras convertidas, executa-o pelos comandos
    genericos (10 passos, timer 13) e recusa um ponteiro fora de sub-rotina ou
    goto.
- [x] A regra de parada conferida nos dados: nos 150 scripts de estado dos
  itens de `ItCo.usd`, uma simulacao da conversao converte 154 trechos (os
  scripts e os alvos de sub-rotina), nao acha ponteiro fora de sub-rotina ou
  goto, e a decodificacao com os comprimentos de `itanimlist.c` termina todos
  em reset (148), return (4) ou goto (2) antes da fronteira, sem opcode
  desconhecido.
- [x] Matching contra `63f7f1874`, token a token, com `-DMUST_MATCH` e sem
  `MELEE_HOST`: `lbcommand.c`, `itanimlist.c`, `ftaction.c`, `lb_013B.c`,
  `ftcolanim.c`, `grmaterial.c` e `lb_0219.c` pre-processam identicos.
- [x] `lb_80014258` passa `ColorOverlay*` como `CommandInfo*` a
  `Command_Execute`. Pelos layouts do host, os campos que os comandos
  genericos tocam ainda coincidem (ponteiro em +8, contagem em +16, retornos a
  partir de +24), com a mesma sobreposicao que o console tem a partir do
  terceiro retorno; nenhuma assercao confere isso ainda.
- [x] Medido: `host-debug` com 187/187 e ctest 15/15 (o novo e o do
  gerador); `host-sanitize`, compilado com clang, com 187/187, ctest 15/15, os
  255 campos batendo tambem ali, rota VS em 19,8 s, nenhum erro do ASan e, do
  UBSan, os mesmos quatro relatos.
- [x] `itPublicData` traduzido (`port/src/game/game_data_translators.c`):
  - `ItemCommonData` (0x160 bytes, palavra a palavra, com os bytes de
    `x48_byte`, `filler_1a` e `filler_1a_2`), as tabelas de `Article` dos 43
    itens comuns, dos 118 de personagem (8 presentes em `ItCo`) e dos 47
    Pokemon, `it_804D6D40_t` e a tabela de 7 animacoes de cor, cujos scripts
    sao convertidos.
  - Cada `Article`: `ItemAttr` com os bits de flag desempacotados a partir do
    bit mais significativo e a cauda escalar palavra a palavra; hurtboxes;
    estados contados ate a proxima fronteira, com animacoes pelo materializador
    e script convertido; modelo com o joint. Objetos que o disco compartilha
    continuam compartilhados.
  - Os 9 campos de estado da Beam Sword e da Fire Flower que pareciam
    ponteiros sem relocacao sao as cadeias dos 6 externos do arquivo
    (`ItmCommonSword_TopN_*` e `ItmCommonFFlower_TopN_ACTION_*`), que
    `lbArchive_InitializeDAT` resolve para NULL; o tradutor os le como NULL.
  - Ficam de fora de proposito os atributos proprios de cada item (layout
    diferente por tipo; 11 blocos tem ponteiros) e a dinamica (so em 3 itens
    comuns; `item.c` le `ItemDynamics` e `itcoll.c` le `ItCollDynamics` sobre
    os mesmos bytes, o que ponteiros de 8 bytes nao conciliam). Os dois campos
    apontam para `melee_host_item_data_left_out`, e `Item_80267978` para com
    nome ao criar um item que tenha um deles.
  - Um teste unitario monta um `ItCo` pequeno e confere, pelos tipos do jogo
    (`port/tests/item_data_check.c`), bytes e palavras de `ItemCommonData`, os
    bits de `ItemAttr`, `Article` compartilhado, a marca, hurtbox, estados com
    script convertido, modelo, `it_804D6D40_t` e a tabela de cor apontando o
    mesmo script.
- [x] Restricoes RObj de bytecode (`REFTYPE_BYTECODE`) no materializador: o
  bytecode fica verbatim, porque `HSD_ByteCodeEval` le um byte por vez e monta
  operandos a partir do mais significativo, e a lista de rvalues vira
  `HSD_RvalueList` em layout host, com os joints pelo materializador. As de
  expressao (`REFTYPE_EXP`) seguem recusadas.
- [x] O leitor C ganhou `melee_host_hsd_reader_joint`, `_anim_joint`,
  `_mat_anim_joint`, `_shape_anim_joint` e `_extent` (bytes ate a proxima
  fronteira).
- [x] No disco: `--load-archive` traduz `itPublicData`; a varredura vai a
  `game_data` 186/186 (`ItCo.usd` e `ItCo.dat`), joints 725/725, com as
  mesmas duas recusas de `TyLight.dat`.
- [x] Matching: `item.c` pre-processa identico sem `MELEE_HOST`.
- [x] Com `GS_VS` na tabela so localmente, a entrada passa pelos itens
  (`Item_80266F70`, `Item_80266FCC`, `it_8026D018`) e pelo audio e cai com
  SIGSEGV em `Ground_801C1E94` (`ground.c:1099`, chamado por
  `Ground_801C0800`), que le `grDatFiles_GetArchive()->unk4`: o `map_head` do
  estagio, ainda sem traducao.
- [x] Medido sem a entrada: `host-debug` com 188/188 e ctest 15/15;
  `host-sanitize` com 188/188, ctest 15/15, rota VS em 19,7 s, nenhum erro do
  ASan e, do UBSan, os mesmos quatro relatos.
- [x] `map_head` traduzido (`port/src/game/game_data_translators.c`), o
  `UnkStageDat` que `grDatFiles_801C6038` guarda:
  - Os modelos primeiro (`UnkStageDat_x8_t`): joint, tabelas de animacao
    terminadas por NULL, camera, `LightList`, fog, `GrJoint`, os bytes de flag
    de animacao (verbatim; `granime.c` os indexa) e a lista `s16` que
    `Ground_801C3FA4` recebe. Depois as tabelas de pares `s16` que
    `Ground_801C5940` percorre, as splines, os overrides de luz e os
    materiais que `grDatFiles_801C6228` marca.
  - Os overrides: `Ground_801C20E0` compara o descritor de cada entrada com as
    luzes dos modelos por endereco. O materializador passou a construir cada
    luz uma vez por endereco, e o tradutor entrega a mesma `HSD_LightDesc`.
    `unk1C` conta o dobro das entradas que a tabela tem, em todos os 66
    estagios em que uma simulacao chegou ate ela, e `find_light_override`
    percorre essa contagem, lendo como entradas as tabelas seguintes e o
    proprio `map_head`. O host monta a mesma contagem; o que nao e luz ganha
    um endereco unico no payload, que nunca iguala uma luz.
  - Os materiais sao os `HSD_MObjDesc` dos proprios modelos (os 27 de Hyrule
    Temple). `UnkStageDatInternal` tem layout host sob `MELEE_HOST`, com o
    `rendermode` depois do nome de classe em largura de ponteiro, onde
    `grDatFiles_801C6228` liga `0x4000000`.
  - Ficam de fora, porque nada no jogo os le: o `x14` de cada modelo (zeros
    em `GrSh.dat`), a tabela `unk20` e o ponteiro de joint que `ground.c`
    declara como preenchimento nas entradas de pares.
  - O leitor C ganhou `melee_host_hsd_reader_camera`, `_fog`, `_light_lists`,
    `_light_built_at`, `_mobj` e `_spline`; `scene_lights` passou a montar a
    mesma tabela de luzes a partir de um offset.
  - Um teste unitario monta um `map_head` e confere, pelos tipos do jogo
    (`port/tests/stage_data_check.c`), o modelo, os pares, os overrides com a
    contagem dobrada e a luz batendo por endereco, e o material pelo layout
    host.
- [x] No disco: 69 dos 71 `map_head` traduzem. `GrIz.dat` e `GrNLa.dat` param
  na animacao de luz que segue um joint, a mesma limitacao de `TyLight.dat`.
  Os modelos do Pokemon Stadium (`GrPs*.dat`) tem campos em cadeias de
  externos, partes compartilhadas entre as transformacoes, que
  `lbArchive_InitializeDAT` resolve para NULL. Varredura: `game_data`
  260/262, joints 725/725, `scene_lights` 17/17 com 51 LObjs.
- [x] Matching: `grdatfiles.c` e `ground.c` pre-processam identicos sem
  `MELEE_HOST`.
- [x] Com `GS_VS` na tabela so localmente, a entrada passa por
  `Stage_8022524C` inteiro, pela camera e por `fn_8016E2BC`, e para em
  `Fighter_LoadCommonData` (`fighter.c:182`), que pede `ftLoadCommonData` a
  `PlCo.dat` (assert de `lbarchive.c:87`). No caminho o jogo imprime "use
  dummy CamRange" e "use dummy DeadRange".
- [x] Medido sem a entrada: `host-debug` com 189/189 e ctest 15/15;
  `host-sanitize` com 189/189, ctest 15/15, rota VS em 19,6 s, nenhum erro do
  ASan e, do UBSan, os mesmos quatro relatos.
- [x] `ftLoadCommonData` traduzido (`port/src/game/game_data_translators.c`):
  o registro de 23 ponteiros que `Fighter_LoadCommonData` copia para globais.
  - So escalares, palavra a palavra: `ftCommonData` (0x818 bytes, com as
    cores e `x6EC` em bytes), as linhas de `ftCo_ItemThrowAttrs` (que
    `ftCo_ItemThrow.c` percorre com offsets do console, validos porque as
    linhas sao so floats), as linhas de swing, os multiplicadores de staling,
    os modificadores de escala, coelho, metal e gravidade, `CrowdConfig`, as
    distancias e alcances da IA de CPU e as listas de ataque
    (`ftCo_AttackEntry`, 0x24 bytes de escalares). Os vetores sem tamanho
    gravado vao ate a proxima fronteira.
  - `ftPartsTable` e `Fighter_804D6540`, por tipo de lutador, apontam bytes
    que ficam verbatim. As duas tabelas de cor tem o formato dos itens, com
    scripts convertidos. `Fighter_804D6534` e o joint e a animacao do pedestal
    de reaparecimento. `Fighter_804D6530` guarda cada lista de `Vec2` seguida
    do tamanho, que `ftCo_DamageFall.c` le de volta de um slot de ponteiro; o
    host guarda ali o tamanho como inteiro. As tabelas de tremor sao uma
    lista de `Vec2` e o tamanho; `6514` e `6504` sao joints; os scripts de CPU
    sao bytes que `ftCo_800B4880` le um a um.
  - `Fighter_804D6510` fica NULL: nada no jogo o le.
  - Um teste unitario monta um `PlCo` pequeno e confere as 23 tabelas pelos
    tipos do jogo (`port/tests/fighter_data_check.c`).
- [x] No disco: `ftLoadCommonData` traduz de `PlCo.dat` (805 relocacoes).
- [x] Com `GS_VS` na tabela so localmente, a entrada passa por
  `Fighter_LoadCommonData` e pelo resto de `Fighter_800679B0` e para em
  `Fighter_Create` -> `ftData_8008572C`, chamado por `Player_80031AD0` em
  `fn_8016E2BC`: `PlFx.dat` nao tem traducao para `ftDataFox` (assert de
  `lbarchive.c:87`).
- [x] Medido sem a entrada: `host-debug` com 190/190 e ctest 15/15;
  `host-sanitize` com 190/190, ctest 15/15, rota VS em 19,7 s, nenhum erro do
  ASan e, do UBSan, os mesmos quatro relatos.
- [x] `ftDataFox` traduzido de `PlFx.dat` por um tradutor C em
  `port/src/game/game_data_translators.c`, registrado pelo nome com a funcao
  dos atributos proprios do Fox (os de cada personagem mudam de layout):
  - So escalares, palavra a palavra: `ftCo_DatAttrs` (0x184 bytes, o ultimo
    em byte), `ftFox_DatAttrs` (0xD4 bytes, com o `ReflectDesc` terminando em
    byte), `x34`, `x38`, a camera (`x3C`), `itPickup` (`x40`), `x50`, as
    bordas de `x44` (seis `s16` e quatro floats) e `x54`, que no host e
    `int*`, porque no disco e um ponteiro relocado que `ftCo_09F7.c` le assim.
  - Tabelas de acao (`xC` e `x14`): `Fighter_WaitAnimData` de 0x18 bytes, com
    nome, offset e tamanho da animacao, script convertido e flags, contadas
    pelo espaco ate a proxima fronteira. O endereco da animacao (`x14`) vira
    `uintptr_t` no host; `ftData_80085FD4_ret`, que le as mesmas entradas, ganha
    o mesmo layout, com os dois bits de flag no topo da palavra, e `ftdata.c`
    carrega esses enderecos em `FT_ANIM_ADDR` (`u32` sem `MELEE_HOST`).
  - Partes (`x8`): `vis_table` em linhas de quatro lookups por modelo, cada um
    uma contagem e `TempS` com indices em bytes, e as linhas de indices `u16`
    de TObj. `x1C` sao conjuntos de partes com bytes e tabela de animacoes;
    `x20` tem joints entre inteiros pequenos, mantidos como estao; `x24` sao
    pares de espera ate a entrada -1, alargados para o `WaitStruct` do host.
  - Dinamica (`x2C`): ossos de 0x18 bytes com registros de 0x3C bytes de
    escalares; dinamica que nomeia `FigaTree` e recusada ate um personagem
    precisar. Hurtboxes, IK e SFX (`FtSFX.x1C` vira ponteiro no host, como no
    disco). Os itens do personagem sao `Article`; o quinto slot do Fox e uma
    lista de inteiros sem ponteiro e fica em bytes.
  - Um teste unitario monta um `PlFx` pequeno e confere pelos tipos do jogo
    (`port/tests/fighter_fox_data_check.c`).
- [x] Animacoes de lutador: `ftData_80085A14` grava em cada acao o endereco da
  animacao em ARAM e `ftData_80085E50` a copia com `lbArq_80014BD0`. O boot do
  host passa a montar os 10 nos de `lbarq.c` (`lbArq_80014D2C`). Sob
  `MELEE_HOST` o no de uma requisicao e achado pelo offset do `ARQRequest`
  dentro dele (o console o guarda no campo `owner`, de 32 bits), a lista pelo
  indice do estado, e a espera sincrona da passos no escalonador
  (`lb_800195D0`), como a leitura de disco. `lbArchiveRelocate` parseia de novo
  a copia de uma animacao que outro lutador ja carregou.
- [x] `map_head`: cada entrada de pares comeca pelo joint do modelo, que
  `Ground_801C34AC` compara por endereco com o joint do modelo para guardar em
  `stage_info.x280` os JObjs que os pares nomeiam, entre eles os pontos de
  partida dos jogadores. O tradutor tratava a palavra como padding; sem os
  pontos, `Ground_801C2D24` nao escrevia a posicao e o lutador nascia com a
  posicao lida da pilha (ECB com NaN, assert de `mpcoll.c:701`). A palavra
  agora vira o mesmo descritor do modelo; 69 de 71 estagios continuam
  traduzindo.
- [x] Com `GS_VS` na tabela so localmente, a entrada cria os dois Fox
  (`fn_8016E2BC` termina) e para em `fn_8016E730` -> `fn_801A1134`
  (`gmpause.c:86`): `GmPause.dat` nao tem traducao para
  `ScGamPause_scene_data`, e `scene->models[0]` cai com `scene` nulo.
- [x] Matching: `ftdata.c`, `ftanim.c`, `ftwaitanim.c`, `fighter.c`,
  `lbarchive.c` e `lbarq.c` pre-processam identicos sem `MELEE_HOST`, token a
  token.
- [x] Medido sem a entrada: `host-debug` com 191/191 e ctest 15/15;
  `host-sanitize` com 191/191, ctest 15/15, rota VS em 21,0 s, nenhum erro do
  ASan e, do UBSan, os mesmos quatro pontos.
- [x] `_scene_data` na API de arquivo: o `SceneDesc` inteiro em layout host
  (`MaterializedSceneDesc`, com o layout de `sc/types.h`). Modelos, cada um com
  joint e as tres tabelas de animacao terminadas por NULL; listas de luz como
  `_scene_lights`; cameras com a tabela de `HSD_CameraAnim`; fogs com a tabela
  de animacoes, das quais o jogo so le o `HSD_AObjDesc` inicial. Cameras e fogs
  sao vetores sem terminador: em `GmPause.dat` a unica entrada de fog e seguida
  pelo proprio `SceneDesc`. O host conta entradas enquanto elas tem descritor
  relocado e ate a proxima fronteira, e sempre reserva uma entrada, para que
  um vetor sem camera leia descritor NULL.
  - `--sweep-archives` e `--load-archive` carregam o tipo como a cena faz:
    todos os joints de modelo, a primeira camera e o primeiro fog e as luzes.
    No disco: os 43 `_scene_data` traduzem e carregam (1.495 objetos);
    `ScGamPause_scene_data` com 16 e `ScInfDmg_scene_data` com 17. As quatro
    recusas da varredura sao as de antes (`map_head` de `GrIz` e `GrNLa` e as
    luzes de `TyLight.dat`).
  - Um teste unitario monta a cena com o formato de `GmPause.dat` e
    `IfAll.dat` e confere pelo `SceneDesc` do jogo
    (`port/tests/scene_data_check.c`).
- [x] Com `GS_VS` na tabela so localmente, a entrada passa pelo menu de pausa
  (`fn_801A1134`), pelo som da torcida e pela cena do HUD e para em
  `ifAll_802F390C` -> `ifStatus_802F7134` (`if_2F6E.c:143`):
  `ScInfCnt_scene_models` nao tem traducao.
- [x] Medido sem a entrada: `host-debug` com 192/192 e ctest 15/15;
  `host-sanitize` com 192/192, ctest 15/15, rota VS em 20,4 s, nenhum erro do
  ASan e, do UBSan, os mesmos quatro pontos.
- [x] `_scene_models` na API de arquivo: uma tabela de `DynamicModelDesc*`
  terminada por NULL, com os modelos montados como os do `_scene_data`.
  `IfAll.dat` nomeia sem sufixo mais quatro tabelas do HUD, reconhecidas pelo
  nome inteiro: `Stc_scemdls` (`ifstock.c`), `Stc_rarwmdls` (`if_2FD9.c`),
  `tdsce` (`iftime.c`) e `lupe` (`ifmagnify.c`). `lupe` parecia um modelo
  solto no levantamento, mas `ifMagnify_802FC3C0` o le por
  `*(DynamicModelDesc**)`: e uma tabela de um, com o registro do modelo logo
  antes dela. No disco os 26 simbolos traduzem e carregam (268 JObjs); em
  `IfAll.dat`, 10 com 84. Teste unitario com tabela de dois modelos, os nomes
  sem sufixo e a tabela de um (`port/tests/scene_data_check.c`).
- [x] `lbBgFlashColAnimData` (`LbBf.dat`): as animacoes de cor do flash de
  fundo, que `lb_0219.c` passa a `lb_800144C8` no formato das dos itens;
  traduzido pelo mesmo codigo.
- [x] Com `GS_VS` na tabela so localmente, `fn_8016E730` termina e a entrada
  para em `gm_Scene_Vs_OnEnter` -> `ifStatus_802F665C` ->
  `ifStatus_802F5EC0` (`ifstatus.c:712`): `ifStatus_802F6194` recebe um JObj
  convertido em GObj e anda por `next_gx` e `next`, que no console caem onde
  o JObj guarda `child` e `next`. No host a largura de ponteiro os separa, e
  `next_gx` le o `parent` da raiz, nulo.
- [x] Medido sem a entrada: `host-debug` com 193/193 e ctest 15/15;
  `host-sanitize` com 193/193, ctest 15/15, rota VS em 20,6 s, nenhum erro do
  ASan e, do UBSan, os mesmos quatro pontos.
- [x] `ifStatus_802F6194` sob `MELEE_HOST` anda pelo `child` e pelo `next` do
  proprio JObj. Sem o define, `ifstatus.c` pre-processa identico, token a
  token.
- [x] Com `GS_VS` na tabela so localmente, `gm_Scene_Vs_OnEnter` termina e a
  cena entra no laco de frames. O primeiro frame roda os procs e para no
  desenho: `ftDrawCommon_80080E18` -> `ftLib_80086A8C` ->
  `Camera_80030CFC` projeta um ponto da caixa de camera de um lutador com x
  fora de +-50.000 (assert de `lbvector.c:383`).
- [x] Medido sem a entrada: `host-debug` com 193/193 e ctest 15/15;
  `host-sanitize` com 193/193, ctest 15/15, rota VS em 20,3 s, nenhum erro do
  ASan e, do UBSan, os mesmos quatro pontos.
- [x] O NaN do primeiro frame vinha da camera, nao do lutador: posicao, caixa
  de camera e ECB estavam validos, mas o olho e o interesse do CObj tinham x e
  y NaN. `Camera_8002AF68` soma `game_camera.translation` ao transform, e um
  watchpoint mostrou `Camera_ApplyQuake` gravando NaN ali. A funcao chega a
  `cm_803BCB64` (a descricao da camera) convertendo `&cm_803BCB18` numa
  struct com a tabela de callbacks, os dois `HSD_WObjDesc` e a descricao, que
  no console ficam em sequencia no `.data`. No host os statics nao ficam em
  sequencia e a tabela de callbacks tem ponteiros de 8 bytes; a largura do
  viewport lida saia zero, a escala infinita e `0 * inf` dava NaN. Sob
  `MELEE_HOST` a funcao le `cm_803BCB64` direto; sem o define `camera.c`
  pre-processa identico, token a token.
- [x] Medido sem a entrada: `host-debug` com 193/193 e ctest 15/15;
  `host-sanitize` com 193/193, ctest 15/15, rota VS em 19,7 s, nenhum erro do
  ASan e, do UBSan, os mesmos quatro pontos.
- [x] Nos procs dos frames seguintes, duas estruturas guardavam enderecos em
  32 bits:
  - As listas de geradores de particula guardam o gerador anterior
    (`hsd_804D78F8`, em `generator.c`) e a cabeca da lista de `HSD_SList`
    (`hsd_804D78F4`, em `particle.c`) em variaveis `u32`; ao remover um
    gerador, `hsd_8039D3AC` escrevia pelo endereco cortado. Sob `MELEE_HOST`
    as duas usam `PS_ADDRESS` (`uintptr_t`, `u32` sem o define), declarado em
    `particle.h`.
  - `ifStatus_PercentOnDeathAnimationThink` le o `IfDamageState` por outra
    struct (`UnkX`), com fillers nos offsets do console; os dois ponteiros do
    inicio de `IfDamageState` movem os campos no host, e `x54_jobj` lia bytes
    errados. Sob `MELEE_HOST` os fillers vem dos offsets de `IfDamageState`, com
    `STATIC_ASSERT` de que `x10_flags`, `x34_vec` e `x54_jobj` caem em `flags`,
    `velocity_x` e `jobjs`.
  - A rota VS sob ASan (`host-sanitize` com `GS_VS` so localmente) achou mais
    duas structs locais lidas sobre statics em sequencia, o mesmo caso de
    `Camera_ApplyQuake`: `lbRefract_800222A4` copia `imagedesc0` por uma
    visao que comeca em `texture_mtx` (estouro de global de 24 bytes), e
    `ftmaterial.c` le os templates de TEV e de constante por
    `struct ft_MObjInfo`, que comeca em `ftMObj`. Sob `MELEE_HOST` o codigo
    nomeia `imagedesc0`, `ftMaterial_803C69D0` e `ftMaterial_803C6A44`.
  - Na mesma rota o ASan achou a escrita que corrompia o heap:
    `mpIsland_8005A728` aloca cada segmento de chao, teto e parede com
    `HSD_MemAlloc(0x2C)`, o `mp_UnkStruct0` do console ate antes de `ptr`. No
    host `next` e `ptr` tem 8 bytes e `x28` ja cai depois de 0x2C; o jogo
    escrevia 2 bytes alem de cada bloco. Sob `MELEE_HOST` os tres pontos
    alocam `sizeof(mp_UnkStruct0)`.
  - `ft_800852B0`, que zera os caches de dados de lutador, chega a
    `ftData_Table_Unk0`, `ftData_UnkIntPairs` e `ft_8045993C` pela distancia
    a `CostumeListsForeachCharacter` (+0x108 e +5940) e a `gFtDataList`
    (+0x84) nas secoes de dados do console, conferida em `symbols.txt`. No
    host isso escrevia 8 bytes alem de `CostumeListsForeachCharacter`. Sob
    `MELEE_HOST` a funcao nomeia os tres.
  - A luz de cada lutador (`ftCo_8009F578`) usa um `HSD_LightDesc` estatico
    cuja posicao aponta cinco floats no lugar de um `HSD_WObjDesc` (nome nulo,
    posicao e RObj nulo, 20 bytes no console). No host o registro tem dois
    ponteiros de 8 bytes, e `WObjLoad` lia o nome dos dois primeiros floats e o
    RObj depois do vetor. Sob `MELEE_HOST` o arquivo declara o
    `HSD_WObjDesc` (`light_position`).
  - As listas de simbolos de `lbArchive_LoadSections`, `LoadSymbols`,
    `80016DBC`, `80017040` e `800171CC` terminam num `0` literal, um `int`.
    No x86-64 um `int` passado depois do sexto argumento vai numa posicao de 8
    bytes da pilha com a metade alta indefinida, e `va_arg` o le como ponteiro:
    sob ASan a fantasia do Fox (`ftData_80085820`) nao achava o fim da lista e
    lia alem dos argumentos. Das 153 chamadas, 104 passam `0` (duas no meio da
    lista, em `ftdata.c`), e a maior lista tem 166 argumentos. Sob
    `MELEE_HOST`, `lbarchive.h` troca as cinco por macros que juntam os
    argumentos num vetor de `const void*`, onde o `0` vira ponteiro nulo, e
    `lbarchive.c` percorre o vetor como o laco original. O arquivo define
    `LB_ARCHIVE_IMPLEMENTATION` para que as proprias definicoes nao passem
    pelas macros.
  - Com as listas corrigidas, a rota sob ASan passa por toda a entrada da cena
    e chega aos procs dos frames. `lbVector_WorldToScreen` declara
    `projMtx` como `Mtx` (3x4) e o passa a `MTXPerspective` e `MTXOrtho`, que
    preenchem 4x4; no console a ultima linha cai na variavel seguinte da pilha,
    no host e estouro. Sob `MELEE_HOST` a matriz e `Mtx44`. O mesmo padrao
    fica em `gm_1832.c:635` (`MTXOrtho` num `Mtx`), fora da rota de VS.
  - `ifstatus.c` converte a cor do dano, calculada em float e que chega a 255,
    direto para `s8`, o que o C deixa indefinido. Sob `MELEE_HOST` a conversao
    passa por `s32`, como o console faz.
  - Com os lutadores ja rodando `Fighter_procUpdate`, o primeiro efeito de
    particula estourou o pool de `HSD_psAppSRT`: `efLib` o cria com
    `psInitAppSRT(0, 0xA4)`, o tamanho do console, e no host a struct tem 184
    bytes (`freefunc` cai no offset 168). Sob `MELEE_HOST` o pool usa
    `sizeof(HSD_psAppSRT)`. Os outros pools com tamanho literal nao precisam
    de correcao: os de `ftdemo.c` e `fighter.c` sao buffers de bytes de
    animacao, `Player_AllocData` nao e usado e os tres de `tev.c` so aparecem
    na tabela de estatisticas de `initialize.c`.
  - Relatos novos do UBSan nessa rota, sem correcao por ora: deslocamento de
    `1` por 31 em `int` (`fighter.c:2105`, `ftCo_Attack100.c:301` e `:315`) e
    o indice 107 de `by_attack_hi[65]` em `plbonus.c:44`, que le os vetores
    seguintes da mesma struct de `u32`.
  - A rota da luta acrescenta tres relatos do UBSan da mesma classe dos quatro
    conhecidos (chamada por ponteiro de funcao de outro tipo):
    `lbrefract.c:46`, `granime.c:517` e o callback de ARQ em
    `baselib_support.c:124`.
  - Sem o define, `particle.c`, `generator.c`, `ifstatus.c`, `lbrefract.c`,
    `ftmaterial.c`, `mpisland.c`, `ftdata.c`, `ftCo_09F4.c`, `lbarchive.c`,
    `lbvector.c`, `eflib.c`, `ifall.c` e `gmvs.c` pre-processam identicos,
    token a token (comparados contra a arvore de HEAD com as linhas de
    compilacao do `build.ninja`).
- [x] Medido sem a entrada: `host-debug` com 193/193 e ctest 15/15;
  `host-sanitize` com 193/193, ctest 15/15, rota VS em 19,7 s, nenhum erro do
  ASan e, do UBSan, os mesmos quatro pontos.

## Em andamento

- [ ] Compilar todo o codigo relevante sem assembly PPC.
- [ ] Resource manager runtime consumindo o manifesto extraido.
- [ ] Cancelamento, streaming e prioridade completa da API DVD.
- [ ] Loader HSD com schemas Disk/Runtime e referencias ciclicas. A API de
  arquivo ja atende joints, animacoes, cameras, luzes, fog, sprites e
  `_scene_data` e `_scene_models`; faltam imagens e paletas soltas, dados de
  estagio e `ftData*` dos outros personagens.
- [ ] Fluxo vertical de luta local: `StartMeleeData` → cena VS → players →
  loop de frame (roteiro em `docs/fight_flow_port.md`). Titulo, menu, CSS e SSS
  ja rodam pelo codigo do jogo e produzem a selecao; falta a cena de luta
  (`GS_VS`, `gm_Scene_Vs_*` em `gmvs.c`) com lutadores, estagio, HUD e camera.
- [ ] Coordenadas de bump (`GX_TG_BUMPn`), os 1,7% de triangulos que o TEV por
  fragmento ainda nao reproduz: exigem a direcao da luz projetada em tangente e
  binormal, e hoje a coordenada de origem passa sem perturbacao.
- [ ] Fog, que o shader ainda nao aplica, e as luzes descritas pela propria
  cena, que o materializador ainda nao traduz.

  A camada formava um unico bloco: os onze arquivos se referenciam
  mutuamente, entao adicionar qualquer um exigia adicionar todos.

  Historico das duas ondas de bloqueio: a primeira comecou em 73 simbolos e a
  segunda em 56. Ambas estao fechadas. `synth.c` tambem ja entrou no build;
  `hsd_audio_stubs.c` conserva somente `HSD_LogInit`, que no host nao precisa
  redirecionar MSL stdio porque `OSReport` ja escreve no fluxo de erro nativo.

## Proximos gates

1. A cena de luta (`GS_VS`): `gm_Scene_Vs_OnEnter`, `OnFrame` e `OnExit` de
   `gmvs.c` na tabela do host, com o `onEnterVs` do estado
   (`gmVsMelee_EnterVs`, que monta o `StartMeleeData`) e o que a cena alcanca:
   lutadores (Fox), estagio (Hyrule Temple, `grshrine.c`), mapa de colisao, HUD
   e camera. O teste `melee-host-vs-selection-asset` ja entrega a selecao que
   ela consome. A cena liga, e com `GS_VS` na tabela so localmente a entrada
   passa pela refracao, pelos efeitos, pelos dados de jogador e pelo estagio
   e para nos dados comuns de item (`itPublicData`). O roteiro, o levantamento
   dos itens (inclusive os scripts de comando em bit-fields, que tambem sao os
   dos lutadores) e o historico das ondas de link estao em
   `docs/fight_flow_port.md`.
2. Texturas de profundidade no presenter: `GX_ZT_REPLACE` com `Z8` e `Z24X8`,
   que o apagamento de tela (`HSD_EraseRect`) e as SObj usam.
3. Tornar a fachada AX/ARAM capaz de executar vozes e streaming, sem ainda
   confundir isso com uma saida DSP real.
4. Fechar o que o TEV por fragmento nao cobre: bump, fog e copias de EFB.

## Limitacoes atuais

- Os assets `GALE01` extraidos estao disponiveis apenas em `assets-local`, que
  permanece ignorado pelo Git e nao faz parte de builds ou artefatos publicos.
- O executavel ainda nao chama `gmMain`.
- AX, CARD, streaming DVD e THP ainda nao estao implementados. AX/AI/ARAM so
  possuem a fachada minima necessaria para `HSD_SynthInit`; PAD e DVD
  assincrono tem pontes basicas; os backends completos ainda faltam.
- O estado GX e registrado, nao rasterizado pelo host: a imagem vem do preview
  SDL/OpenGL, que desenha a geometria capturada com o programa TEV de cada draw
  avaliado num shader gerado.
- O runtime GObj executa processos e ja pode possuir objetos graficos reais,
  mas as cenas so sao apresentadas pelos diagnosticos `--view-*`, nao pelo
  laco de frame do jogo.
- O preview segue culling, profundidade, blend, mascara de cor e as duas alpha
  compare, e a cor vem do TEV por fragmento. Ele nao aplica fog, nao le mipmaps
  (a minificacao usa o filtro de magnificacao), nao modela TEV indireto nem
  `GXSetTevSwapModeTable` (usa as tabelas do `GXInit`, e o recorder para com
  nome se o jogo instalar outra) e chama as funcoes GL 2.0+ por `GL_GLEXT_PROTOTYPES`, o que so
  resolve no Linux.
- A iluminacao continua por vertice, como no GX; o que e por fragmento e o TEV.
  Um modelo solto e iluminado pelas luzes substitutas, nao pelas do estagio,
  entao a cor de um personagem no preview nao e a de uma luta.
- Na captura em espaco de mundo o joint do Mario aparece de cabeca para baixo
  no preview, enquanto trofeus e cenas aparecem na orientacao certa. A camera
  reproduz a mesma sequencia `glFrustum`/`glRotate` de antes; a causa nao foi
  investigada.
- `GX_BM_LOGIC` e `GX_CULL_ALL` nao sao modelados pelo preview: o primeiro cai
  para sem blend, o segundo descarta o grupo. Nenhum simbolo do disco usa os
  dois, entao isso nunca foi exercitado por dado real.
- A convencao de winding do preview e a do GX, sentido horario como face
  frontal, e nao foi verificada visualmente. A tecla F inverte, porque um
  modelo aparecendo do lado de dentro e a evidencia mais clara de que a
  suposicao esta errada para um asset.
- A animacao de personagem roda pela arvore de JObj, nao por um `Fighter`. O
  mapeamento de osso e posicional: o n-esimo no da lista cai na n-esima junta
  em ordem de construcao. `ftanim.c` faz o mesmo percurso, mas pulando partes
  por flags do lutador, entao quando o runtime de luta entrar esse mapeamento
  precisa passar por ele em vez da ordem crua.
- Um `HSD_AObjDesc.obj_id` retira uma referencia para o JObj que nomeia. Se o
  asset apontar para o proprio JObj que possui o AObj, cria um ciclo de
  ownership; o teste sintetico o libera explicitamente antes de desmontar a
  arvore. Assets reais devem manter essa referencia em um objeto externo, como
  o runtime original espera.
- Um simbolo de joint solto nao tem cena e portanto nao tem camera; nesse caso
  o render usa a camera substituta do host. A linha de relatorio diz qual das
  duas foi usada.
- `melee_host_scene_graphics_render` desenha um modelo da cena por chamada, que
  e a unidade que o jogo usa (um GObj por modelo). Uma cena de varios modelos
  precisa de uma chamada por indice.
- O materializador de descritores recusa, com mensagem propria, o que ainda
  nao sabe traduzir: joints de particula, animacao de luz que segue um joint
  pela chave da tabela de IDs (as duas tabelas de luz de `TyLight.dat`),
  descritor de render de material (`HSD_MObjDesc.renderdesc`, cuja forma so o
  setup customizado conhece) e restricoes RObj de expressao, que guardam
  endereco de funcao do console. Os joints de spline e as restricoes de
  bytecode ja traduzem.
- Os descritores materializados sao validados campo a campo, mas os payloads
  GX entregues aos loaders originais sao ponteiros crus. Um array de vertice
  nao tem tamanho conhecido pelo descritor, entao so a base e verificada: a
  partir dai o host confia no asset, como o console confiava. As display lists
  sao a excecao, porque a contagem de blocos da o tamanho exato.
- Os descritores vivem enquanto o handle da cena vive. Os objetos carregados
  apontam para dentro deles, como apontariam para um arquivo ainda residente
  no heap do console, e por isso `melee_host_scene_graphics_release` libera a
  arvore antes dos descritores.
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
- A API de arquivo do host nao ve o heap do jogo. Um buffer liberado sem ser
  parseado de novo deixa seus descritores vivos ate
  `melee_host_hsd_archive_release`, entao a memoria cresce por arquivo
  carregado ate o endereco ser reutilizado. Quando o fluxo de cena entrar, a
  destruicao dos heaps de cena precisa liberar o que foi parseado dentro deles.
- `HSD_ArchiveLocateExtern` com endereco nao nulo e recusado: ligar o extern
  exigiria escrever um ponteiro host num descritor que so existe quando o
  simbolo e pedido. O codigo compilado so chama com NULL.
- `ftdata.c` separa a animacao guardada em ARAM da guardada em memoria pelo
  endereco abaixo de `0x80000000`. No host o endereco de memoria vem do
  alocador do host, que em x86-64 fica acima desse limite; um host que aloque
  abaixo de 2 GB confundiria os dois.
- O materializador ignora o ponteiro de parametros de uma luz infinita e a
  posicao de uma luz ambiente, porque `LObjLoad` nao os le; os arquivos do
  disco trazem o primeiro relocado.
- `HSD_MemAlloc` no host usa o alocador do host, nao o heap do OS que
  `HSD_InitComponent` cria. Destruir os heaps de uma cena, que no console
  libera tudo o que ela alocou, nao libera objetos HSD no host; cada troca de
  cena vai vazar ate `HSD_MemAlloc` passar a usar o heap OSAlloc corrente.
- A espera de disco do host so da passos no escalonador. Tela de erro de
  drive, reset e cartao nao existem. Uma leitura que falha marca o erro
  estatico do devcom, que nao chama o callback, e o jogo espera para sempre; a
  flag e `static` e o host ainda nao a enxerga.
- O boot ainda pula `GXInit` (a FIFO e reservada na arena, mas nao entregue)
  e `lbMthp_8001F87C`. O nivel de depuracao de um disco de desenvolvimento nao
  e selecionado.
- Nenhum efeito sonoro pode tocar: o host nao monta descritores de amostra, os
  fluxos de comando do `.sem` continuam big-endian e os nucleos de reverb e
  chorus param se o mixer os chamar. O carregador de SFX de `synth.c` precisa
  de um port de verdade quando o mixer existir. Ate la, deflag e descarga de
  banco voltam o banco a cabeca, porque os grupos nao sao registrados.
- Os alarmes seguem o relogio de parede, a nao ser que o host congele o
  relogio do OS. Congelado, o tempo so anda na espera de `lb_800195D0`, e so
  quando a fila bruta de pad esta vazia: uma espera por outro alarme com
  amostras de pad na fila nao avanca. Nada apresenta os frames a 60 Hz de
  relogio de parede ainda.
- A tabela de modos e cenas do host tem o titulo, o menu principal e o modo VS
  com as duas cenas de selecao. O modo VS do host termina na luta, sem
  resultados, morte subita nem desafiante (`gmvsmode.c` sob `MELEE_HOST`).
  Pedir um modo fora da tabela e recusado antes de o jogo seguir o NULL que
  acharia, e uma cena fora da tabela encerra o modo; nos dois casos
  `--run-modes` termina o roteiro com `stopped:`.
- A CSS e a SSS foram conferidas pelo estado do jogo (portas, fichas,
  personagem e estagio escolhidos), nao pela imagem: `--view-title-scene` so
  apresenta o titulo, e nenhum frame dessas cenas foi desenhado numa janela.
- O pool SIS do host tem o dobro do tamanho que a cena pede. Isso garante que
  cada bloco cabe no dobro do que ocupava no console, mas a fragmentacao pode
  ser outra; um "Memory Empty" em outra cena deve ser medido com o retrato do
  pool (blocos usados e livres) antes de mexer no fator.
- Sem cartao de memoria so os 14 personagens e os estagios iniciais estao
  liberados. Final Destination e Battlefield ficam travados na SSS, e a
  primeira luta mira Hyrule Temple.
- O modo VS do host nao tem CPU nem handicap conferidos: o roteiro so abre
  portas HMN. Os caminhos de regras, troca de nome e botoes de time da CSS
  alcancam paradas com nome em `unported.c`.
- Os arquivos da demo do titulo carregam, mas nada os usa ainda. Os bancos de
  particula que eles trazem chegam ja localizados pela API de arquivo do host.
  `particle.c` para com nome diante de um banco de formas ou de uma tabela de
  referencia; os efeitos e os estagios passam os dois nulos.
  `grDatFiles_801C5FC0`, que parseia o arquivo de estagio, agora e o codigo
  original e ainda nao foi executado.
- A ARQ do host nao modela as duas filas de prioridade nem a divisao em
  pedacos da SDK: tudo completa no passo seguinte, na ordem postada. Postar
  sem backend ativo para com nome.
- `x4` de cada nivel de evento fica NULL. E o parametro proprio de cada evento
  (dois ints, uma contagem de moedas, floats, uma lista de personagens, uma
  lista de ints e, em um nivel, um int e um ponteiro), lido no lugar pelo codigo
  do evento, e precisa de traducao por evento antes de o modo de eventos entrar
  na tabela.
- O caminho do cartao de memoria guarda enderecos em 32 bits: os parametros de
  `lb_8001BB48`, `lb_8001BC18`, `lb_8001BE30` e `lb_8001BF04`, os campos
  `unk_18` e `unk_1C` da tarefa e a fila de comandos de `hsd_3A94.c`, onde
  `hsd_3B27.c` converte ponteiros para `s32` 14 vezes. Sem cartao a cadeia de
  tarefas para na sondagem (`CARD_RESULT_NOCARD` vira `0xF`, que a tarefa
  seguinte nao aceita) e nada disso e lido; um cartao virtual precisa desse
  caminho em largura de ponteiro.
- Sem voz no host a musica nao comeca. O parse do cabecalho `.hps`
  (`HSD_SynthPStreamHeaderCallback`) le campos big-endian como nativos, e nada
  chama o callback de quadro do AX: quando o host entregar vozes, o stream
  precisa das duas coisas.
- `ScNtcCommon_scene_data`, o aviso de acesso ao cartao, nao e traduzido e fica
  NULL; `lb_8001CF18` confere e nao cria a cena.
- Os opcodes 8 e 9 do texto SIS param com nome, e com eles o push de cursor em
  `string_buffer`, que tambem guardaria um endereco em 32 bits.
- Com um frame sink instalado, a captura do GX so vale ate o `GXCopyDisp`
  seguinte; quem precisa dela depois tem de copiar dentro do sink.
- `db_TakeScreenshotIfPending` passa o endereco do XFB como `int` para
  `hsd_80393A5C`, o que trunca um ponteiro de 64 bits. So roda com um
  screenshot pendente, que `db_CheckScreenshot` marca nos niveis de
  depuracao.
- O ctest nao falha por relato do UBSan, que so imprime; confira com
  `ctest --preset host-sanitize -V`. Restam quatro, todos de chamada por um
  ponteiro de funcao de outro tipo: `FogRelease` pelo ponteiro de release de
  `class.h` e `HSD_JObjRemoveAll` por `gobjobject.c`, no sistema de classes do
  HSD, e, desde que a CSS e a SSS rodam, `HSD_AObjStopAnim` passado a
  `HSD_ForeachAnim` (`aobj.c:301`) e o callback de render `fn_8026407C` de
  `mncharsel.c` (`gobj.c:154`).
- O presenter nao modela texturas de profundidade (`GXSetZTexture`), e o
  decodificador nao conhece `Z8`, `Z16` nem `Z24X8`. O apagamento de tela do
  titulo desenha seu quad com a profundidade da propria geometria, na metade
  entre near e far, em vez de limpar para o far; so esconde o que estiver
  alem dessa metade. As SObj que usam `GX_ZT_REPLACE` tem o mesmo desvio.
- A janela, o ritmo de 60 Hz e a entrada de teclado e gamepad de
  `--view-title-scene` foram escritos, mas so o caminho escondido, que grava o
  BMP, foi executado e conferido; o que muda na janela e a troca de buffer e a
  leitura de eventos.
- `--view-title-scene` termina quando `gm_801A4D34` retorna: START encerra a
  cena, e a seguinte nao existe no host ainda.
- O cache de texturas do titulo e o presenter reconhecem uma imagem pelo
  endereco dos dados e da paleta; uma animacao que reescreva uma imagem no
  mesmo endereco continua mostrando a primeira.
- A tabela `stage_datas` de `ground.c` liga todos os estagios, mas nenhum
  carrega ainda por inteiro: a API de arquivo do host traduz `grGroundParam` e
  `map_head` (69 de 71), mas nao `coll_data`, `itemdata`, `ALDYakuAll`,
  `yakumono_param`, `map_plit` nem `quake_model_set`. Os 71 arquivos
  `Gr*.dat` trazem os quatro primeiros; 67 trazem os dois ultimos.
- `lbFile_800164A4` escolhe leitura direta em RAM porque o destino esta acima
  de `0x80000000`, o que os enderecos do host em 64 bits satisfazem; a
  separacao entre ARAM e RAM de `lbmemory.c` usa 16 MB no host.
- O jogo tem 59 listas variadicas de ponteiros terminadas por `0`; 10 delas usam
  `VA_END_PTR` (`gmTitle_801A1AC0`, `lb_80014534` e as oito do caminho do
  menu). Todas as unidades ja compilam no core, entao o que falta nao aparece
  no build: cada caminho novo que alcance uma das outras 49 precisa da mesma
  troca, e sob ASan o sintoma e escrita num endereco com os 32 bits baixos
  zerados.
- Parte da matematica de ponto flutuante ainda e da glibc, e nao do jogo: o
  `melee-pc` resolve `atanf`, `sinf`, `cosf`, `tanf`, `sqrtf`, `sqrt` e `fmodf`
  pela `libm`. O `atanf` de `lbtrigf.c` so compila sob `__MWERKS__`, porque usa
  o intrinseco `__fnmsubs` do PowerPC, e as outras vem da MSL, que o host nao
  compila. Para o determinismo contra o console essas funcoes precisam dar o
  mesmo resultado bit a bit. `lbtrigf.c` tambem le floats por `*(u32*) &f`,
  o que funciona no build de debug e pede `-fno-strict-aliasing` ou `memcpy`
  num build otimizado.
