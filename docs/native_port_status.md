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
  `--view-joint FILE SYMBOL`. O joint do Mario rende 6.328 triangulos com 31 de
  31 texturas decodificadas, e `GmPause.dat` 130 triangulos com 9 de 9.
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
- [x] Reducao simbolica do programa TEV a forma que da para renderizar direto.
  Um unico estagio que escreve o registrador final com a aritmetica neutra e
  lido como `textura x cor`, `konst x cor`, `textura` ou `cor`, com a constante
  de alpha que o HSD deixa no primeiro registrador. Qualquer outra coisa resolve
  como aproximada, o que nao e falha: e o consumidor sendo obrigado a dizer que
  escolheu um substituto.
- [x] Cobertura medida, nao suposta: dos 3.368.616 triangulos do disco,
  **676.893 (20%) tem o material lido exatamente** e o resto e aproximado. A
  maioria dos programas tem mais de um estagio - 1112 de um estagio contra 2574
  de dois a oito na amostra levantada. Os numeros aparecem em cada render, por
  simbolo.
- [x] O preview agrupa por par de estado de pixel e programa de material, nao
  so por estado de pixel, porque um triangulo precisa dos dois para ser
  desenhado como foi capturado. Onde o programa foi lido exatamente ele aplica
  a forma correta; onde nao, cai em textura x cor de vertice, que e o que o
  visualizador sempre fez.
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
- [ ] Avaliar os programas TEV de varios estagios, que sao 80% dos triangulos.
  Isso depende de duas coisas que ainda nao existem, e nenhuma e pequena:
  avaliacao por fragmento, porque o TEV e por pixel e o preview e de funcao
  fixa, e os dois canais de cor rasterizada separados, porque os programas de
  varios estagios referenciam `COLOR0A0` e `COLOR1A1` de forma independente
  enquanto a captura guarda uma cor por vertice. A segunda exige avaliar a
  equacao de iluminacao do GX a partir dos controles de canal, das luzes e das
  normais, que o host modela mas nao executa.

  A camada formava um unico bloco: os onze arquivos se referenciam
  mutuamente, entao adicionar qualquer um exigia adicionar todos.

  Historico das duas ondas de bloqueio: a primeira comecou em 73 simbolos e a
  segunda em 56. Ambas estao fechadas. O que restava por ultimo eram
  `HSD_LogInit`, que no host nao tem o que redirecionar porque a saida ja passa
  por `OSReport`, e `HSD_Synth_804D6018`, o handle do heap de audio. Os dois
  vivem em `port/src/game/hsd_audio_stubs.c` como definicoes reais, nao como
  armadilhas, e devem sair de la quando `synth.c` e `debug.c` entrarem no
  build.

## Proximos gates

1. Executar a equacao de iluminacao do GX para produzir as cores rasterizadas
   por canal, que e o que falta antes de qualquer avaliacao TEV honesta de
   varios estagios.
2. Inicializar o primeiro grafo de audio sem DSP/ARAM fisico, o que resolve
   tambem os dois simbolos que hoje vivem em `hsd_audio_stubs.c`.
3. Ligar o laco de frame: `HSD_GObj_RunProcs` para a simulacao e o retrace de
   VI para a apresentacao, com o executavel chamando o fluxo em vez de um
   diagnostico.

## Limitacoes atuais

- Os assets `GALE01` extraidos estao disponiveis apenas em `assets-local`, que
  permanece ignorado pelo Git e nao faz parte de builds ou artefatos publicos.
- O executavel ainda nao chama `gmMain`.
- AX, CARD, streaming DVD e THP ainda nao estao implementados. PAD e DVD
  assincrono tem pontes basicas; os backends completos ainda faltam.
- O estado GX e registrado, nao rasterizado. Nenhum pixel e produzido por ele:
  o que existe de imagem vem do preview SDL/OpenGL sobre geometria decodificada
  separadamente.
- O runtime GObj executa processos e ja pode possuir objetos graficos reais.
  As cenas do disco carregam e desenham pela camada de objetos, mas nenhuma foi
  apresentada: o recorder GX registra a geometria e nao produz pixels.
- O preview e um renderer OpenGL proprio alimentado pela geometria e pelo
  estado capturados. Ele segue culling, profundidade, blend, compare de alpha e
  mascara de cor, e a cor de 20% dos triangulos vem do programa de material
  lido; nos outros 80% ela e uma aproximacao declarada.
- A cor rasterizada capturada e a cor de vertice do display list, nao a saida
  dos canais de iluminacao do GX. Enquanto a equacao de iluminacao nao for
  executada, material, luzes e ambiente nao influenciam a imagem.
- `GX_BM_LOGIC` e `GX_CULL_ALL` nao sao modelados pelo preview: o primeiro cai
  para sem blend, o segundo descarta o grupo. Nenhum simbolo do disco usa os
  dois, entao isso nunca foi exercitado por dado real.
- A convencao de winding do preview e a do GX, sentido horario como face
  frontal, e nao foi verificada visualmente. A tecla F inverte, porque um
  modelo aparecendo do lado de dentro e a evidencia mais clara de que a
  suposicao esta errada para um asset.
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
