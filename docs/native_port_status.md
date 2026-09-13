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
  `_scene_lights`, `_fog`, `_sobjdesc`, `_figatree`). Sufixo sem traducao e
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
  e `lbArchiveRelocate` recusa no host em vez de relocar. Verificado: as quatro
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
- [x] Presets de debug/sanitizers e workflow multiplataforma.

## Em andamento

- [ ] Compilar todo o codigo relevante sem assembly PPC.
- [ ] Resource manager runtime consumindo o manifesto extraido.
- [ ] Cancelamento, streaming e prioridade completa da API DVD.
- [ ] Loader HSD com schemas Disk/Runtime e referencias ciclicas. A API de
  arquivo ja atende joints, animacoes, cameras, luzes, fog e sprites; faltam
  `_scene_data` (a tabela de fogs de `SceneDesc` nao e o array terminado por
  NULL que o schema supunha: seguir as entradas cai em valores nao relocados),
  imagens e paletas soltas, dados de estagio, `ftData*` e `SIS_*`.
- [ ] Fluxo vertical de luta local: `StartMeleeData` → cena VS → players →
  loop de frame (roteiro em `docs/fight_flow_port.md`). O escalonador de frame
  ja roda; falta a camada de objetos graficos que alimenta os callbacks de
  render.
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

1. Tela de titulo montada pelo codigo original: compilar `gmtitle.c` e rodar
   `gm_Scene_Title_OnEnter`, que ja tem o arquivo, a memoria e o disco de que
   precisa. Faltam as fachadas que ele chama: audio (`lbAudioAx_*`), filme
   (`lbMthp_8001F614`), texto (`HSD_SisLib_*`), `lbspdisplay.c`
   (`lb_80013B14`, `lb_80011AC4`, `lb_80011E24`) e as rotinas de menu e de
   demo do titulo; e o terminador `0` da lista variadica, que precisa ser NULL
   em 64 bits.
2. Laco de frame desenhando pelos GX links dos GObjs e apresentado pelo
   SDL/OpenGL com a camera e a projecao do jogo, em vez da camera orbital do
   preview.
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
  `GXSetTevSwapModeTable` (usa as tabelas do `GXInit`, que o codigo compilado
  nunca troca) e chama as funcoes GL 2.0+ por `GL_GLEXT_PROTOTYPES`, o que so
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
  nao sabe traduzir: joints de spline e de particula, descritor de render de
  material (`HSD_MObjDesc.renderdesc`, cuja forma so o setup customizado
  conhece) e restricoes RObj de expressao e de bytecode, que guardam endereco
  de funcao do console. Nenhum dos 647 simbolos de cena e de joint do disco
  bate nesses casos.
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
- `lbArchiveRelocate`, que `ftdata.c` usa para animacoes mantidas em ARAM,
  ainda reloca no lugar com aritmetica de 32 bits e precisa de um caminho host
  antes de `ftdata.c` entrar no build.
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
- O boot pula `lbAudioAx_8002838C` (a ARAM comeca mais baixo que no console),
  `lbDvd_80018F68` (nao ha cache de preload) e `GXInit` (a FIFO e reservada na
  arena, mas nao entregue). `lbdvd.c` nao compila: `lbArchive_80016F80`,
  `lbArchive_80017040`, `lbArchive_800171CC` e `lbFile_800168A0` citam o cache
  de preload e so ligam enquanto ninguem os chama. No build com sanitizers,
  `lbarchive.c` e `lbfile.c` compilam sem instrumentacao pelo mesmo motivo.
- `lbFile_800164A4` escolhe leitura direta em RAM porque o destino esta acima
  de `0x80000000`, o que os enderecos do host em 64 bits satisfazem; a
  separacao entre ARAM e RAM de `lbmemory.c` usa 16 MB no host.
- Listas variadicas terminadas por `0`, como a de `gmTitle_801A1AC0`, sao
  lidas de volta como ponteiro por `va_arg`; em 64 bits isso e comportamento
  indefinido. Quando essas unidades entrarem no build, o terminador precisa
  ser NULL sob `MELEE_HOST`.
