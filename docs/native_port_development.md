# Desenvolvimento do port nativo

O build host e independente do build matching PowerPC. Ele ainda e uma base de
bootstrap: nao inicia o jogo completo.

## Build rapido

Requisitos atuais:

- CMake 3.25 ou superior;
- Ninja;
- compilador C17/C++20;
- Python 3.10 ou superior para ferramentas e testes.

```sh
cmake --preset host-debug
cmake --build --preset host-debug
ctest --preset host-debug
./build/host-debug/port/melee-pc --diagnose
```

No Linux, a build com ASan e UBSan e:

```sh
cmake --preset host-sanitize
cmake --build --preset host-sanitize
ctest --preset host-sanitize
```

## Inventario de portabilidade

O inventario e heuristico e serve para acompanhar reducao de bloqueadores:

```sh
python3 tools/port_inventory.py --output build/port-inventory.json
```

O JSON separa codigo do jogo/baselib da SDK Dolphin e lista objetos matching,
assembly, APIs de plataforma, asserts de layout, enderecos fixos e casts para
32 bits. Uma ocorrencia inventariada nao significa necessariamente um bug; ela
indica trabalho que precisa ser classificado.

## Inspecao e extracao de disco

Inspecionar um dump sem extrair:

```sh
python3 tools/melee_extract.py inspect /caminho/para/melee.iso
```

Extrair uma copia `GALE01` suportada:

```sh
python3 tools/melee_extract.py extract /caminho/para/melee.iso assets-local
```

A ferramenta valida o magic GameCube, game ID, limites do DOL/FST e o SHA-1 do
`main.dol`. Arquivos existentes nao sao sobrescritos sem `--force`. O diretorio
extraido contem `manifest.json`, `dvd-index.bin`, `sys/main.dol` e os arquivos do FST. Ele contem
material do disco do usuario e nunca deve ser versionado ou publicado.

## Escalonador de cena

O runtime de objetos HSD original pode ser executado sem assets. O comando cria
dois objetos de cena, pausa o p_link de um deles e roda o escalonador:

```sh
./build/host-debug/port/melee-pc --diagnose-scene-runtime 60
```

Ele reporta quantos frames rodaram, quantos objetos e processos estao vivos e
quantas vezes cada processo foi chamado, alem de retraces VI e callbacks de
draw-done. O processo do p_link pausado deve terminar com zero chamadas; os
outros tres contadores devem ser iguais ao numero de frames pedido.

## Heap do OS

`OSAlloc.c` e `OSArena.c` sao codigo da SDK e carregam enderecos em largura de
ponteiro apenas sob `MELEE_HOST`. Ao mexer neles, confirme que o caminho
matching nao muda comparando o codigo gerado com `unsigned long` de 32 bits,
que e a largura do PowerPC:

```sh
cc -m32 -c -O2 -std=c17 -Iextern/dolphin/include -Isrc -w \
   extern/dolphin/src/dolphin/os/OSAlloc.c -o /tmp/novo.o
```

Compare com o mesmo comando sobre a versao anterior do arquivo. Sem `-m32` a
comparacao nao vale: fora do host, `u32` e `unsigned long`, que tem 64 bits em
x86-64 e 32 no alvo.

## Tempo de video

A camada VI do host nao dorme. Um retrace acontece quando o jogo bloqueia em
`VIWaitForRetrace` ou quando o laco do host chama
`melee_host_video_advance_retrace`. Escolha um dos dois como fonte do tempo;
usar os dois no mesmo laco faz o contador de retrace avancar em dobro.

## Inspecao de HSD

Um arquivo HSD individual pode ser validado sem fazer relocacao in-place:

```sh
./build/host-debug/port/melee-pc --inspect-hsd assets-local/path/file.dat
```

O parser mantem offsets serializados de 32 bits separados de ponteiros runtime,
que e a base para o loader 64-bit.

## Arquivos pelo caminho do jogo

O jogo nao abre um arquivo por offset: chama `lbArchive_LoadSymbols` com uma
lista de nomes, que faz `HSD_ArchiveParse`, resolve os externs e pede cada
simbolo a `HSD_ArchiveGetPublicAddress`. O host implementa essas quatro
funcoes em `port/src/assets/hsd_host_archive.cpp`. O comando abaixo repete
esse caminho com a lista que `gmTitle_801A1AC0` usa e entrega cada descritor
ao loader original do seu tipo:

```sh
./build/host-debug/port/melee-pc --load-archive assets-local/GmTtAll.usd \
    TtlMoji_Top_joint TtlMoji_Top_animjoint TtlMoji_Top_matanim_joint \
    TtlMoji_Top_shapeanim_joint ScTitle_cam_int1_camera ScTitle_scene_lights \
    ScTitle_fog TtlBg_Top_joint TtlBg_Top_animjoint TtlBg_Top_matanim_joint \
    TtlBg_Top_shapeanim_joint TitleMark_sobjdesc
```

Sem nomes, o comando pede todos os simbolos publicos do arquivo. A varredura
faz isso para todo arquivo do disco que e um unico arquivo HSD:

```sh
./build/host-debug/port/melee-pc --sweep-archives assets-local
```

Tres leituras do relatorio:

- `translated` sem `loaded` e o esperado para animacoes e sprites: uma arvore
  de animacao so carrega junto da arvore que dirige, e o sprite precisa da
  biblioteca de sprites, que ainda nao compila.
- `unsupported` nao e falha. E um simbolo cujo sufixo o host ainda nao
  traduz, e o jogo receberia NULL com o relatorio `host HSD archive: cannot
  translate`. So `failed` conta como erro.
- O tipo vem do sufixo do nome. Ao adicionar um tipo, acrescente o sufixo em
  `kSuffixes`, com os mais longos antes (`_matanim_joint` antes de `_joint`),
  e o metodo correspondente no materializador. Quando o layout no disco for
  duvidoso, levante-o antes em todos os arquivos que tem o sufixo: foi assim
  que a tabela de fogs de `SceneDesc` se mostrou diferente do que o header
  sugere.

A camada guarda o que parseou pelo endereco do buffer, nao pelo `HSD_Archive`.
Um teste que parseia um arquivo deve chamar `melee_host_hsd_archive_release`
com o mesmo buffer ao terminar, senao os descritores sobrevivem ao teste.

## Boot de memoria e o carregador do jogo

O comando abaixo sobe a memoria como o `gmMain` e carrega o arquivo da tela de
titulo pelo `lbArchive_LoadSymbols` original:

```sh
./build/host-debug/port/melee-pc --boot-title-archive assets-local
```

O caminho e o do jogo: `lbFile` pede a leitura a fila devcom, que abre o
arquivo no DVD do host, e espera em `lb_800195D0`. No host essa espera da um
passo no escalonador, e so entao a leitura acontece.

Tres cuidados:

- Rode fora do binario de testes. A sequencia move a arena do OS e recria os
  heaps do HSD, o que quebraria testes que supoem o bootstrap headless.
- Se o comando parar com 100% de CPU, a espera de disco nao esta vendo a
  leitura terminar. A causa que ja apareceu foi o DVD recusando o intervalo:
  o devcom marca um erro estatico, nao chama o callback e `waitForDisc` gira.
  Um `gdb` interrompido mostra `lb_800195D0` no topo; confira o intervalo
  pedido contra o tamanho do arquivo.
- Ao mexer em `lbmemory.c`, `lbheap.c`, `lbfile.c` ou `lbarchive.c`, compare os
  objetos de 32 bits antes e depois com `-DMUST_MATCH`, compilando as duas
  versoes a partir do mesmo caminho (o `__FILE__` entra nos dados):

```sh
cc -m32 -c -O2 -std=c17 -DMUST_MATCH -include stdint.h -include stddef.h \
   -include stdbool.h -Iextern/dolphin/include -Isrc -Isrc/melee/lb -w \
   /tmp/match/lbmemory.c -o /tmp/match/lbmemory.o
```

Sem `-DMUST_MATCH` o `HSD_ASSERT` usa `__LINE__`, e qualquer linha inserida
muda o objeto sem mudar o DOL. Os `-include` suprem o `stdint.h` que o
toolchain matching traz por outro caminho.

## Cena de titulo

```sh
./build/host-debug/port/melee-pc --boot-title-scene assets-local
```

Sobe o boot do `gmMain`, le os dados que vivem no `main.dol` e roda
`gm_801A4BD4` e `gm_Scene_Title_OnEnter`. O relatorio conta os GObjs pelas
listas da propria biblioteca e sai com erro se a cena nao tiver o que
`gmtitle.c` monta.

Como a cadeia foi aberta, e como abrir a proxima:

- Coloque o entrypoint que se quer alcancar sob uma chamada real no
  executavel. Com `--gc-sections`, funcao sem chamador e descartada junto com
  as referencias dela, e o link nao diz nada.
- Leia as referencias indefinidas com `LANG=C`, agrupe por arquivo de origem e
  decida caso a caso. Preferencia: compilar o modulo original; dado que vive no
  DOL, ler do `main.dol`; tabela que cita conteudo inteiro do jogo (estagios,
  lutadores), referencia `weak` quando o jogo ja trata a entrada nula; funcao
  so alcancavel por um caminho que a cena nao toma, parada com nome em
  `port/src/game/unported.c`.
- Um arquivo de dados que o codigo le direto como struct (`.ssm`, `.sem`) e
  big-endian e costuma relocar ponteiros de 32 bits no lugar. Levante o layout
  no disco antes de escrever o caminho do host, como na API de arquivo.
- Um `assert` que falha logo depois de uma leitura de disco quase sempre e
  ordem de bytes; um segfault em alocador de biblioteca quase sempre e
  inicializacao do `gmMain` que o boot do host ainda nao faz.
- Rode a mesma cadeia no `host-sanitize` antes de dar o recorte por fechado.
  Dois erros que o build de debug atravessava em silencio so apareceram la:
  `long` onde o console tem 32 bits (a SDK de audio usa `long` para amostras;
  troque por `s32`/`u32`, o mesmo tipo na build PowerPC) e codigo que atravessa
  objetos vizinhos de `.bss` como se fossem uma struct, confiando na ordem do
  DOL. Para o segundo, confira o intervalo em `config/GALE01/symbols.txt` e,
  sob `MELEE_HOST`, junte os objetos numa definicao so, com macros nos
  deslocamentos originais, como em `toy.c`.

## Laco de frame da cena

```sh
./build/host-debug/port/melee-pc --run-title-scene assets-local
```

Congela o relogio do OS, sobe o boot, entra no titulo e roda `gm_801A4D34`
ate a cena pedir para sair. Sai com erro se nao forem os 621 frames da
contagem e do tempo limite do titulo, sem botoes e com frames desenhados.

- Congele o relogio antes do boot (`melee_host_os_time_freeze`). Congelado, o
  tempo so anda quando o jogo espera o proximo alarme em `lb_800195D0`, e cada
  execucao repete os mesmos frames. Sem congelar, os alarmes seguem o relogio
  de parede e o laco gira enquanto espera.
- Uma espera que nao termina quase sempre e uma interrupcao que o host nao
  entrega. No console elas chegam no meio de qualquer espera; no host chegam
  em pontos escolhidos: alarmes em `lb_800195D0`, draw done em
  `VIWaitForRetrace` e `GXWaitDrawDone`. Leia o que a espera testa e procure
  quem mudaria aquele estado.
- Uma cena que desenha todo frame precisa de um frame sink
  (`melee_host_gx_set_frame_sink`), senao a captura do GX cresce sem limite.
- Codigo de depuracao alcancavel pelo laco (`gm_801A4970`, screenshot, USB)
  so roda sob uma condicao de `DbLevel` ou de evento. Confira a condicao antes
  de portar; se o host nunca a satisfaz, pare com nome em `unported.c`.

## Carga de cena pela camada de objetos

O comando abaixo materializa os descritores do arquivo em layout host e chama
`HSD_JObjLoadJoint`, que e o mesmo entrypoint que toda cena do jogo usa. Ele
reporta o que os loaders originais construiram:

```sh
./build/host-debug/port/melee-pc --load-scene \
    assets-local/GmPause.dat ScGamPause_scene_data
```

O terceiro argumento opcional escolhe o modelo dentro de `SceneDesc.models`.
Um simbolo publico que nomeia um joint direto entra pelo outro comando:

```sh
./build/host-debug/port/melee-pc --load-joint \
    assets-local/PlMrNr.dat PlyMario5K_Share_joint
```

As contagens de PObj e de blocos de display list podem ser comparadas com as
que `--inspect-pobj` produz pelo schema de leitura separado; as duas rotas leem
o mesmo arquivo por caminhos independentes, entao divergencia entre elas e
sinal de erro em uma das duas.

Ao estender o materializador, prefira recusar um campo que ainda nao sabe
traduzir a adivinhar sua forma. Um ponteiro errado entregue aos loaders
originais aparece muito depois, longe da causa.

## Animacao

O comando abaixo carrega um modelo, anexa uma animacao por
`HSD_JObjAddAnimAll` e avanca N frames com `HSD_JObjAnimAll`, relatando quantas
juntas se moveram:

```sh
./build/host-debug/port/melee-pc --animate-joint \
    assets-local/GmTtAll.dat TtlMoji_Top_joint \
    assets-local/GmTtAll.dat TtlMoji_Top_animjoint TtlMoji_Top_matanim_joint 200
```

Use `-` no lugar de um simbolo que o arquivo nao tem. O arquivo de animacao
pode ser outro: e assim que um personagem guarda o modelo e os movimentos
separados.

Duas leituras do relatorio evitam um diagnostico errado:

- `AObj frame` e a unica prova de que a animacao avancou. Ele conta N-1 para N
  chamadas, porque a primeira interpretacao usa taxa zero devido a
  `AOBJ_FIRST_PLAY`.
- `joints moved` pode ser zero com a animacao rodando perfeitamente: animacao
  de material muda cor e textura sem mexer no esqueleto. Olhe o frame antes de
  concluir que nada funcionou.

As animacoes de personagem ficam em um arquivo por personagem que guarda
varios arquivos HSD enfileirados, um por acao. Liste e toque por nome:

```sh
./build/host-debug/port/melee-pc --list-animations assets-local/PlMrAJ.dat
./build/host-debug/port/melee-pc --animate-named \
    assets-local/PlMrNr.dat PlyMario5K_Share_joint \
    assets-local/PlMrAJ.dat PlyMario5K_Share_ACTION_WalkMiddle_figatree 45
```

Esse caminho nao usa as arvores HSD: um personagem usa `FigaTree`, o formato
proprio do Melee, e `lbAnim_8001E6D8` o aplica direto a um `HSD_JObj`. O
mapeamento de osso e posicional, entao um modelo e uma animacao de personagens
diferentes vao anexar sem erro e produzir lixo; confira que os prefixos dos
simbolos combinam.

Um AObj recem-carregado ja toca a um frame por chamada, porque `HSD_AObjAlloc`
deixa `framerate` em 1.0. Definir a taxa serve para escolher outra velocidade
ou o sentido inverso, que e o que o jogo faz por acao de lutador.

## Render pela camada original

Os mesmos dois comandos com `--render-` em vez de `--load-` desenham a arvore
por `HSD_JObjDispAll`, nas tres passagens do callback de render original, e
relatam o que chegou ao recorder GX:

```sh
./build/host-debug/port/melee-pc --render-scene \
    assets-local/GmPause.dat ScGamPause_scene_data
./build/host-debug/port/melee-pc --render-joint \
    assets-local/PlMrNr.dat PlyMario5K_Share_joint
```

Duas linhas do relatorio valem mais que a contagem de triangulos:

- `display list errors` diferente de zero significa que o stream que o codigo
  original entregou ao GX nao e um que o host consegue seguir.
- `rejected vertex indices` diferente de zero significa que um indice caiu fora
  do array e o atributo foi descartado, entao a captura esta incompleta. Isso
  nao aparece na geometria: o vertice so fica sem aquele atributo. Trate como
  erro, nao como aviso.

O espaco em que os vertices saem depende da view que o comando pede. Os
comandos `--render-` usam a camera da cena, como o jogo, e entregam espaco de
vista. Os comandos de preview pedem view identidade, o que deixa as matrizes
carregadas no GX como transformacoes de mundo, porque quem move a camera ali e
o proprio visualizador:

```sh
./build/host-debug/port/melee-pc --view-scene \
    assets-local/GmPause.dat ScGamPause_scene_data
./build/host-debug/port/melee-pc --view-joint \
    assets-local/PlMrNr.dat PlyMario5K_Share_joint
```

A janela mostra a geometria que o caminho de display original desenhou. Cada
draw e colorido pelo seu programa TEV avaliado por fragmento, com as texturas de
todos os mapas que os estagios amostram e as coordenadas que o texgen original
gerou. `--view-pobj` continua mostrando o que o schema de leitura decodifica por
conta propria, com um estagio MODULATE no lugar do material, e serve de segunda
opiniao quando as duas imagens divergem.

Os comandos `--render-` listam os estados capturados, um por linha. Vale ler
essas linhas antes de culpar a geometria: `blend=1` com `zwrite=0` e o modo
translucido normal do HSD, `cull=0` marca um objeto de duas faces, e `alpha`
diferente de `7@0` significa que o material recorta por alpha.

A linha `TEV evaluated per fragment` diz quantos triangulos tem o programa TEV
reproduzido por inteiro e quantos usam algo que o port ainda nao modela; a
linha `tev N` de cada programa diz o que falta (`unmodelled=bump texgen`). O
preview gera um shader GLSL por programa distinto a partir de
`port/src/gx/tev.cpp`, com a aritmetica inteira do hardware, e registradores e
konst entram como uniforms, entao o mesmo material com outra cor nao gera outro
shader.

Ao mexer no avaliador ou no gerador, rode a conformidade. Ela desenha cada par
de programa TEV e estado de pixel da captura num alvo de um pixel, com cores
rasterizadas e texels aleatorios, e exige que o pixel lido seja o que
`melee::gx::evaluate_tev` e o alpha test calculam, descarte incluido:

```sh
./build/host-debug/port/melee-pc --tev-conformance-joint \
    assets-local/PlMrNr.dat PlyMario5K_Share_joint
./build/host-debug/port/melee-pc --tev-conformance-scene \
    assets-local/GmPause.dat ScGamPause_scene_data
```

Uma divergencia significa que o GLSL e a referencia discordam. A referencia e a
que tem testes unitarios contra a formula do hardware, entao comece suspeitando
do gerador.

Para obter uma imagem sem abrir janela, defina `MELEE_HOST_SCREENSHOT` com um
caminho `.bmp`; o comando `--view-` renderiza um quadro fora da tela e sai:

```sh
MELEE_HOST_SCREENSHOT=/tmp/mario.bmp ./build/host-debug/port/melee-pc \
    --view-joint assets-local/PlMrNr.dat PlyMario5K_Share_joint
```

Um modelo solto nao tem as luzes do estagio. O render da fachada registra uma
luz ambiente e uma infinita pelo `HSD_LObj` original, como ja fazia com a
camera substituta. Sem elas todo material iluminado sai preto: o ambiente do
canal e o ambiente do material multiplicado pela luz ambiente corrente, e sem
luz ambiente ele vale zero. Um preview preto com conformidade limpa quase sempre
e isso, e nao o shader.

Um detalhe que confunde quem for mexer nisso: `HSD_TExpSetReg` monta os valores
de registrador em um array local nao inicializado e escreve so os componentes
que a expressao nomeia. O que sobra e lixo de pilha que chega ao GX. Nao afeta a
imagem, porque nenhum estagio le esses componentes, mas qualquer comparacao de
estado TEV precisa ignora-los, senao o mesmo material conta como varios e a
contagem muda entre builds e entre execucoes.

Na janela, F inverte a face frontal. A convencao adotada e a do GX, sentido
horario como frente, e nao foi verificada visualmente: se um modelo aparecer do
lado de dentro, essa tecla e o primeiro teste.

Ao comparar com `--inspect-pobj`, lembre que as duas rotas nao cobrem o mesmo
conjunto. O schema percorre todos os modelos da cena e desenha tudo; o caminho
original desenha um modelo por chamada e pula objeto escondido e PObj que
descarta as duas faces. Compare so em cena de um modelo, e confira a linha
`not drawn` antes de concluir que a divergencia e um bug.

Para testar a ponte DVD contra um arquivo realmente extraido sem iniciar o
jogo, leia uma amostra do recurso pelo nome de FST:

```sh
./build/host-debug/port/melee-pc --read-resource assets-local DbCo.dat
```

## Regras durante o bootstrap

- `configure.py` e o build matching continuam sendo o oraculo PowerPC.
- Codigo host usa `MELEE_HOST`; mudancas sem esse define nao podem alterar o DOL.
- Codigo novo do port trata warnings como erro.
- Codigo decompilado usa warnings de legado, sem reformatacao ou casts cosmeticos
  que prejudiquem matching.
- Nenhum asset do jogo entra no repositorio ou nos artefatos publicos de CI.
