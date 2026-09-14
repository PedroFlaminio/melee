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
  lutadores), ligada forte, porque a decomp inteira esta no core e uma
  referencia `weak` nao puxa membro de biblioteca estatica (a entrada fica
  nula sem aviso); funcao
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

## Apresentacao pela janela

```sh
./build/host-debug/port/melee-pc --view-title-scene assets-local
./build/host-debug/port/melee-pc --view-title-scene assets-local /tmp/f.bmp 120
```

A primeira forma abre a janela e segue ate a cena sair ou ate Esc. A segunda
desenha escondida, grava o frame pedido e imprime a captura dele: as texturas
com formato e tamanho, as views e as runs na ordem do jogo, cada uma com blend,
alpha compare, programa TEV, texturas e a caixa que cobre na tela.

- Para achar o que esta errado numa imagem, case a regiao com a caixa das
  runs. Os retangulos brancos do titulo eram as runs 10 e 15, ambas com
  textura I4, e o erro estava no decodificador, nao no shader: a conformidade
  do TEV sorteia texels, entao nao cobre o que o decodificador entrega.
- Uma cena pelo avesso, ou que some, e culling; confira o front face antes de
  mexer na projecao.
- O presenter guarda uma textura GL por endereco de imagem, e o cache do
  titulo decodifica cada textura uma vez pelo endereco dos dados e da paleta.
  Uma animacao que reescreva uma imagem ou uma paleta no mesmo endereco
  precisa de outra chave.

## Modos de jogo em sequencia

```sh
./build/host-debug/port/melee-pc --run-modes assets-local 0 1
./build/host-debug/port/melee-pc --run-modes assets-local 0 2 \
    120:START 160:DOWN 200:A 240:A
```

Comeca o roteamento do gerenciador de cenas no modo pedido (`0` e o titulo) e
roda ate N modos, um por vez, como o laco de `gm_801A4510`: o modo corrente
passa a anterior e o pendente a corrente (`gm_HostBeginGameModes` e
`gm_HostRunCurrentGameMode`, sob `MELEE_HOST` em `gm_1A3F.c`). Cada modo vem da
tabela do host (`port/src/game/game_tables.c`, no lugar de `gmscdata.c`), com o
preload do estado, o `on_enter`, a cena, o laco de frame, o `onExit` que escolhe
o proximo modo e a espera do cartao de memoria.

Cada entrada do roteiro e `FRAME[-ULTIMO]:ENTRADA[+ENTRADA][@PORTA]`. A
entrada e um botao (A, B, X, Y, Z, L, R, START, UP, DOWN, LEFT, RIGHT) ou
`SX=N`/`SY=N` para o stick principal, de -128 a 127. Sem `-ULTIMO` ela e
segurada por tres frames desenhados; com ele, ate aquele frame inclusive. Os
frames sao contados atraves dos modos. A porta vai de 1 a 4 e e 1 quando
omitida, e uma porta citada no roteiro fica conectada desde o primeiro frame.

Cada modo imprime uma linha, precedida de uma linha por cena de estado que
rodou. No fim saem `vs selection:` (o estagio e o personagem de cada slot
aberto no `VsModeData`), `scenes:` e `route:`. Um modo que a tabela nao tem, ou
um estado cuja cena ela nao tem, encerra o roteiro com `stopped:`. Os testes
conferem essas linhas inteiras, frames incluidos.

O roteiro do teste `melee-host-vs-match-asset` atravessa a selecao do VS com
dois pads, entra na luta e sai dela:

```sh
./build/host-debug/port/melee-pc --run-modes assets-local 0 3 \
    120:START 160:DOWN 200:A 240:A \
    300-315:SY=127 300-315:SY=127@2 320:A 320:A@2 \
    330-337:SX=127 330-333:SX=-127@2 \
    345-354:SY=127 345-354:SY=127@2 360:A 360:A@2 \
    380:START 420-421:SX=-127 425-439:SY=127 445:A \
    680:START 700-715:L+R+A 705-715:START 730-850:B
```

Na CSS as portas comecam fechadas, inclusive a de um pad conectado. Cada pad
sobe o cursor ate o botao HMN da propria porta e aperta A, pega a ficha no
caminho ate o retrato da Fox e a solta com A; START so vale com o banner de
pronto. Na SSS o cursor comeca em (0, -13) e sobe ate Hyrule Temple.

Na luta a pausa so vale depois que o HUD liga (frame 655, depois do GO) e
dez frames depois de pausar. L+R+A+START sai como no contest quando um dos
quatro chega recem-apertado com os quatro seguros; o roteiro segura L+R+A e
aperta START por cima. Sem a tela de resultados, o modo volta a CSS, e B
segurado ali leva ao menu. A luta nao termina sozinha no tempo de um teste
(passou de 1.368 frames sem acabar, e o `host-debug` desenha uns 13 frames
por segundo), entao todo roteiro que entra nela precisa desse caminho de
saida.

Uma entrada `FRAME:BMP=arquivo` grava aquele frame desenhado num BMP, pelo
mesmo presenter escondido de `--view-title-scene`, e imprime triangulos, views
e texturas da captura. Pode haver varias; a rota sai com erro se uma nao for
gravada. Para olhar a imagem, converta com `magick f.bmp f.png`:

```sh
./build/host-debug/port/melee-pc --run-modes assets-local 0 3 \
    ... 640:BMP=/tmp/go.bmp 695:BMP=/tmp/pausa.bmp
```

- O frame e o global da rota, o mesmo das entradas de botao. A luta do teste
  vai do 533 ao 707, e a CSS do 243 ao 383; o 400 ja e SSS.
- Uma textura que o decodificador recusa imprime `texture N (format 0x..) not
  decoded` com o motivo, e o presenter a troca por uma textura branca. Antes de
  procurar geometria errada atras de quadrilateros brancos, confira se o mesmo
  frame imprimiu esse aviso.
- A captura e lida depois do ultimo draw do frame. Um estado que o draw
  consulta e que outro draw pode mudar precisa ser copiado para a captura no
  inicio do draw; consultado na leitura, ele devolve o do ultimo draw. Foi o
  caso da paleta sob um nome de TLUT (`melee_host_gx_captured_texture_tlut`):
  o sintoma era "TLUT index exceeds palette" com paletas de tamanho sem
  relacao com o formato da textura, como 16 entradas para uma C8.
- Uma funcao matching pode escrever por uma variavel que um caminho raro deixa
  sem atribuir. No console o registrador ainda guarda o valor de antes; no
  host o build de debug cai. Foi `fn_8001E60C`, com uma parte de lutador so de
  trilhas de translacao. No crash, leia as variaveis locais (`info locals` no
  gdb) antes de suspeitar dos dados: um ponteiro que aponta para dentro de uma
  funcao, como `HSD_AObjAlloc+81`, e sinal disso.
- Um union de um escalar com bit-fields (`UnkFlagStruct`: `u8 byte` e
  `b0`..`b7`) guarda cada bit numa posicao no console e noutra no host. Uma
  escrita ou leitura pelo escalar com valor diferente de zero muda de sentido;
  sob `MELEE_HOST`, declare os bits na ordem inversa, como nos scripts de
  comando. O sintoma nao e crash, e uma flag que fica 0: foi o que impedia os
  lutadores de serem desenhados. Zerar pelo escalar nao depende da ordem.
- O recorder GX guarda cada tipo de matriz onde o GX guarda: posicao e textura
  na memoria de matrizes, normal (3x3) a parte. Um PObj iluminado carrega
  posicao e normal no mesmo `GX_PNMTXn`, e juntar as duas faz o vertice perder
  a translacao da camera. O sintoma e geometria gigante colada na tela. Antes
  de suspeitar do esqueleto, confira no relatorio do `BMP=` as caixas das
  sequencias de draw e, no gdb, as matrizes dos joints: aqui as duas coisas
  apontaram para lados diferentes, e o erro estava entre elas.
- Uma textura que o jogo preenche com `GXCopyTex` (sombra, refracao e a
  copia de `tobj.c`) fica no host com o que havia na memoria, porque a copia
  de EFB ainda nao produz pixels. O sintoma e uma area preta ou suja onde a
  textura e aplicada. Para confirmar, encha o destino com uma cor fixa dentro
  de `GXCopyTex`, sem commit, e veja se a area muda: foi assim que a faixa
  preta da luta se mostrou a sombra.

- O preload do estado de titulo (`lbDvdPreload_3`) mantem todos os heaps de
  preload, e o `on_enter` da cena registra os arquivos da demo do titulo:
  lutadores, estagio e efeitos. Eles carregam em segundo plano enquanto o
  titulo roda, pelo devcom, e os heaps 4 e 5 ficam em ARAM. E por isso que o
  modo alcanca `ftdata.c`, os arquivos de cada personagem e a ARQ, que a cena
  sozinha nao alcancava.
- Um callback que o console entrega por interrupcao nao pode rodar dentro da
  chamada que o dispara. O devcom posta a ultima transferencia de ARAM e so
  depois desliga o pedido; com a ARQ completando dentro de `ARQPostRequest`, o
  callback devolvia o pedido a lista livre antes, a fila apontava para ela e
  um pedido ja liberado voltava a rodar. O sintoma foi o assert de
  `devcom.c:36` varios frames depois. O que o host completa por conta propria
  entra por `melee_host_dvd_schedule_backend_task`, no passo seguinte.
- Um valor do host que some entre ser gravado e ser lido quase sempre e escrita
  de outro objeto de `.bss`. Um watchpoint de hardware acha quem escreveu:
  `break` onde o valor ja esta certo e, ali, `watch -l variavel`. Foi assim que
  apareceu `tydisplay.c` dimensionando um vetor de ponteiros como
  `0xB0 / sizeof(HSD_Archive*)`: 44 entradas no console, 22 no host, e o laco
  que o limpa escreve 43.
- No `host-sanitize`, rode com `ASAN_OPTIONS=detect_leaks=0`, como o ctest
  faz. O boot e as cenas nao liberam o que alocam, o LeakSanitizer encerra o
  processo com codigo 1 e, com a saida redirecionada, o relatorio do comando
  se perde porque o buffer de `std::cout` nao e esvaziado.
- Um simbolo cujo layout so um header C do jogo descreve e traduzido em C: os
  `types.h` dos modulos nao compilam como C++ (um membro chamado `u8` muda o
  sentido do tipo). O tradutor e registrado pelo nome
  (`melee_host_hsd_register_translator`) em
  `port/src/game/game_data_translators.c`, le o arquivo pelo leitor C da API de
  arquivo, que confere offsets, relocacoes e campos nulos, e preenche os tipos
  do proprio jogo campo a campo, bit-fields incluidos. Levante o layout no disco
  antes: a tabela de eventos tinha um parametro por evento, cada um com forma
  propria, e o que nao tem forma unica fica fora com o motivo escrito.
- Texto SIS e big-endian no disco e nos buffers que o jogo monta, e o
  interpretador lia palavras no lugar. Uma leitura `*(u16*)` ou `*(s16*)` de
  stream vira `HSD_SisLib_ReadU16` ou `HSD_SisLib_ReadS16` sob `MELEE_HOST`.
- Scripts de comando (lutador, item, sobreposicao de cor) chegam ao jogo pela
  API de arquivo ja convertidos para a ordem nativa
  (`melee_host_hsd_reader_command_stream`), e o jogo os le pelas structs de
  `port/src/game/host_command_layout.h`. Ao mudar uma struct de comando ou a
  union `ColorOverlay_x8_t` em `lb/types.h`, rode
  `port/tools/gen_host_command_layout.py`, que regenera o header e o check em
  C; o ctest `melee-host-command-layout-generated` acusa quando isso ficou para
  tras. Uma leitura por cast de `u8`, `u16` ou `s16` do script vira `CMD_U8`,
  `CMD_U16` ou `CMD_S16`; a palavra `u32` inteira nao muda. Um ponteiro no
  script so existe como operando de sub-rotina ou goto, e no host e a
  distancia ate o alvo (`rel`).
- O gerador so le `lb/types.h`. Uma struct de bit-fields sobre a palavra do
  script declarada em outro lugar precisa da ordem inversa escrita a mao sob
  `MELEE_HOST`: `itAnimlistCmdUnk` (`itanimlist.c`) e `gmScriptEventDefault`
  (`ft/types.h`), pela qual `ftaction.c` le o opcode. Esquecida, ela nao
  quebra na leitura: um opcode tirado dos bits baixos que valha 0 e Reset, que
  so encerra o script, e o erro aparece longe, num comando de laco ou de
  sub-rotina que roda sem pilha.
- Toda lista variadica de ponteiros terminada por `0` precisa de `VA_END_PTR`
  quando a unidade entra no build. No build de debug o `0` pode passar por
  sorte; sob ASan a metade alta do slot vem suja e o carregador escreve num
  endereco com os 32 bits baixos zerados, como `0x55ae00000000`.
- Endereco guardado em `int` nao da crash onde e truncado, da um valor pela
  metade mais adiante. Antes de alargar um lado, siga o valor ate onde ele e
  usado: no cartao de memoria o endereco das imagens passa de `int` em `int` ate
  a fila de comandos de 32 bits, e so a gravacao, que exige cartao, o le.
- Converter float fora da faixa para `u8` e comportamento indefinido, e o UBSan
  acusa. O console fica com o byte baixo, que e o que `(u8) (s32)` da.
- O stick chega ao jogo depois do clamp do pad: 127 vira 80. O cursor da CSS
  anda (80² - 200) × 0,0002 = 1,24 por frame, e o da SSS (80 - 30) × 0,03 =
  1,5. Um roteiro de menu com cursor e contado em frames a partir disso, e
  movimentos em um eixo por vez evitam o clamp octogonal da diagonal.
- Nao calibre um roteiro pela imagem. Um script do gdb com `break` no
  `OnFrame` da cena e `commands` que imprimem o estado (cursor, portas, ficha,
  ou o estagio sob o cursor com `call lb_8000B1CC(jobj, 0, $v)` para a posicao
  de mundo de cada icone) mostra frame a frame o que a entrada fez. Foi assim
  que apareceram o clamp e a porta fechada. Rode-o com
  `gdb -batch -ex 'set $arg_from = N' -x script.gdb --args ...`.
- "Memory Empty" em `sislib.c` e o pool de texto SIS da cena. Antes de mexer
  no tamanho, tire o retrato do pool no panic: um comando Python do gdb que
  percorre `used_head` e `free_head` somando `size` mostra quantos blocos, de
  que tamanhos e quanto sobra. O pool do host ja e o dobro do pedido.
- Uma cena que a tabela do host nao tem encerra o modo antes do preload do
  estado, entao `on_enter` do estado nao roda: dados que ele montaria (o
  `StartMeleeData` da luta, por exemplo) ainda nao existem quando o roteiro
  para. Leia a selecao por `melee_host_vs_selection_get`.
- Display lists, arrays de vertice, imagens e keyframes continuam big-endian no
  host. Codigo do jogo que le esses payloads na CPU, e nao pelo GX, precisa
  montar os valores dos bytes: foi o caso da shape animation em `pobj.c`, que
  copiava floats com `memcpy`. O sintoma nao e crash, e geometria com
  coordenadas absurdas ou NaN; o UBSan acusou mais adiante, na conversao para
  `u8` da iluminacao do host. Ao ver NaN numa captura, suba ate quem produziu o
  valor antes de proteger a conversao.
- Um `global-buffer-overflow` do ASan numa tabela do jogo costuma ser leitura
  que o console faz alem do fim e que cai no objeto seguinte do DOL. Confira o
  tamanho em `config/GALE01/symbols.txt`, veja qual simbolo vem depois e leia
  os bytes do `main.dol` extraido pelo endereco (o cabecalho do DOL da offset,
  endereco e tamanho de cada secao). Se so alguns campos sao lidos, uma
  entrada extra sob `MELEE_HOST` com esses bytes reproduz o console, como na
  tabela de estagios da SSS; se o codigo atravessa objetos inteiros, junte-os
  numa definicao so, como em `toy.c`.
- Todo `.c` de `src/melee` ja esta no core (menos `gmscdata.c`), entao alcancar
  uma cena nova nao pede fonte nova no CMake. O que aparece no link e outra
  coisa: um "multiple definition" contra `unported.c` quer dizer que o modulo
  real passou a ser puxado, e a parada deve sair; uma referencia indefinida
  costuma ser SDK, baselib fora do core ou dado que so existe no DOL. Codigo
  original que nao pode rodar no host como esta, como uma relocacao de 32 bits
  no lugar, para com nome dentro do proprio modulo sob `MELEE_HOST`, com o
  motivo, como `psInitDataBankLocate`.
- Um tipo de funcao que o GCC recusa (`incompatible-pointer-types`) quase sempre
  e declaracao e definicao discordando. Antes de escolher o lado, veja o que
  quem chama passa e o que o corpo faz com o valor: `on_demo_init` parecia
  `bool` em quase todos os estagios, mas Final Destination compara o parametro
  com 26.
- O `host-sanitize` descarta no link o mesmo que o `host-debug`, e todo
  modulo compilado e instrumentado. Se um modulo que ninguem chama passar a
  exigir simbolos so no build sanitizado, confira se
  `-fsanitize-address-globals-dead-stripping` e `-Wl,-z,start-stop-gc`
  continuam chegando ao compilador e ao linker: sem elas os metadados de
  globais do ASan mantem vivo tudo o que o modulo cita. Nao volte a excluir
  arquivos da instrumentacao; quando a lista de excecoes saiu, apareceram um
  estouro de pilha e leituras fora de vetor que ela escondia.

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
