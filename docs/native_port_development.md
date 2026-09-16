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
- A tela de resultados so termina quando os quatro jogadores estao prontos
  (`fn_80178050`): CPU e porta vazia ficam prontos sozinhos, e cada humano com
  START na propria porta, depois que a animacao do painel passa do quadro 50.
  Cada START de humano alterna entre pronto e nao pronto, entao um segundo
  toque desfaz o primeiro. Um roteiro com START so na porta 1, ou com dois
  START por porta, fica parado nos resultados para sempre, sem erro e sem
  imprimir nada, porque `--run-modes` so escreve quando uma cena ou um modo
  termina. Para ver onde uma rota parou, rode-a sob gdb com
  breakpoints que imprimem e continuam (`commands` ... `continue`) nas funcoes
  de entrada e no proc da cena.
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
- O mesmo vale para um union de `s32` com bit-fields gravado inteiro a partir
  dos dados: `fighter.c` copia `x10_animCurrFlags` de cada acao para
  `fp->x594_s32` e le a flag de repeticao, as mascaras de partes e o tipo da
  FigaTree pelos campos. No host a animacao da corrida parava no fim e o
  script, com todos os timers ja vencidos, girava sem fim criando efeitos. O
  sintoma e o processo crescendo centenas de MB por segundo num frame que nao
  termina; nao e falta de memoria. Rode rotas longas amostrando `VmRSS` e
  matando por PID acima de um limite, e pare no gdb no frame anterior com
  `N:FIGHTERS` (breakpoint em `melee_host_match_fighter_position`) para ver o
  backtrace das alocacoes.
- Para conferir que um arquivo da decomp continua com os mesmos tokens sem
  `MELEE_HOST`, pre-processe-o duas vezes com as flags do build tiradas de
  `ninja -C build/host-debug -t compdb` sem o define: na arvore de trabalho e
  num `git worktree` do HEAD, com os `-I` trocados para a raiz de cada arvore.
  Troque o caminho do worktree pelo do repositorio antes de comparar, porque os
  asserts embutem `__FILE__`. Um bloco novo antes de um assert desloca o
  `__LINE__` do ramo que nao e MWCC; o ramo do MWCC passa a linha explicita,
  mas prefira pôr macros novas num header para o arquivo manter as linhas.
- O recorder GX guarda cada tipo de matriz onde o GX guarda: posicao e textura
  na memoria de matrizes, normal (3x3) a parte. Um PObj iluminado carrega
  posicao e normal no mesmo `GX_PNMTXn`, e juntar as duas faz o vertice perder
  a translacao da camera. O sintoma e geometria gigante colada na tela. Antes
  de suspeitar do esqueleto, confira no relatorio do `BMP=` as caixas das
  sequencias de draw e, no gdb, as matrizes dos joints: aqui as duas coisas
  apontaram para lados diferentes, e o erro estava entre elas.
- Uma textura que o jogo preenche com `GXCopyTex` sai do rasterizador da CPU
  (`melee_host_gx_copy_efb_to_i4` para a sombra, `..._to_texture` para as
  cores), que desenha a captura do frame ate a copia e aplica as limpezas
  pedidas antes dela. `FRAME:EFBCOPY` decodifica as copias em cor que o frame
  usa e exige uma com pelo menos 16 cores, e um breakpoint em
  `HSD_ImageDescCopyFromEFB` imprime quantas copias ha, de que tamanho e onde.
  Uma area preta ou suja onde a textura e aplicada aponta para um formato que o
  rasterizador nao faz ou para geometria que a captura nao tem. Para confirmar
  que a area e uma copia, encha o destino com uma cor fixa dentro de
  `GXCopyTex`, sem commit: foi assim que a faixa preta da luta se mostrou a
  sombra.
- Uma funcao pequena que devolve `int` pode estar lendo um ponteiro pelo
  layout do console: `mn_80231634` devolve o `child` de um JObj como o `int`
  em +10. O sintoma e SIGSEGV num JObj de endereco com cara de 32 bits
  (`0x5702d640`) logo depois da chamada; procure casts como
  `(HSD_JObj*) mn_80231634(...)`.
- Nos resultados de uma luta concluida, o estado 2 (`fn_80177920`) aceita
  qualquer botao de humano e para no primeiro. Um roteiro que aperte START nas
  duas portas no mesmo frame so passa o vencedor, e a tela espera sem fim:
  aperte um botao antes e START nas portas depois.
- Uma rota longa sem traco nao diz em que frame esta. `--run-modes` imprime
  cada cena ao comecar (`scene 0xNN from frame N`), e um `N:RULES` a cada 50
  frames serve de marcador barato para medir o ritmo, amostrado junto com o
  `VmRSS`.
- O relogio da luta so desce de um em um segundo, e a regra mais curta que o
  menu oferece e um minuto, entao uma rota que espere o tempo acabar leva
  7.200 frames de luta. `FRAME:CLOCK` imprime o relogio (`119s+3`, segundos e
  frames dentro do segundo) e `FRAME:CLOCK=N` o deixa em N segundos: a cena
  segue contando dali e termina a luta sozinha, com o tempo esgotado em
  `0s+59`, que e onde `gm_GetMatchOutcome` le o time-up. O relogio so anda
  com o HUD ligado (frame 655 nas rotas), e a entrada responde `no timer`
  numa luta sem relogio. A rota do teste de morte subita e a longa, sem o
  atalho, dao a mesma sequencia de cenas e o mesmo desfecho; a longa leva 49 s
  no build `-O2`.
- O teclado da janela so e exercitado por evento de verdade.
  `port/tools/play_keyboard_probe.py` abre o `--play` no X11, espera a luta
  comecar (le a linha `scene 0x02 from frame N` da saida, que so sai a tempo
  com `stdbuf -oL`, porque num pipe o stdout do jogo sai em bloco) e manda a
  tecla com `xdotool keydown --window`, que vai so para aquela janela e nao
  passa pelo foco do desktop. Duas entradas `FIGHTERS` e duas `ACTION` dizem
  o que o lutador fez:

```sh
DISPLAY=:1 python3 port/tools/play_keyboard_probe.py --press --key d
DISPLAY=:1 python3 port/tools/play_keyboard_probe.py --press --key j --hold 0.3
DISPLAY=:1 python3 port/tools/play_keyboard_probe.py --no-press
```

  Com `d` o lutador anda 109 a 131 unidades e passa por `Dash` (20), com `j`
  fica no lugar e entra em `Attack11` (44), e sem tecla fica em `Wait` (14)
  onde nasceu. Procure a janela pelo PID do processo, nao pelo nome: uma
  execucao anterior deixa o titulo na lista do servidor X por um tempo e o
  envio para um id morto e um BadWindow que o jogo nunca ve.
- `port/tools/play_keyboard_match.py` joga a rota inteira assim: o titulo, o
  menu, a CSS, a SSS, a luta, a pausa com a saida por L+R+A+START e os
  resultados ate voltar a CSS, com o lado do pad 1 todo em tecla de verdade
  (o pad 2 fica no roteiro, porque o teclado da janela e so o pad 1). O
  relogio nao serve de referencia: cada tecla sai quando chega a linha
  `rules frame N` do frame anterior, e o `keyup` na linha do ultimo frame em
  que a tecla devia estar baixa, o que reproduz `330-337:SX=127` como oito
  frames exatos. Com o `keyup` um frame tarde o cursor da CSS andava 1,24
  unidade a mais e a rota escolhia o Ness. `--shot arquivo.png` grava a
  propria janela com `import -window`, que e a unica imagem possivel aqui: as
  entradas `BMP=` precisam do presenter escondido, que nao tem janela para
  receber tecla.
- O fog entra entre o TEV e o blend, nos dois caminhos que desenham a
  captura, e `MELEE_HOST_FOG=0` captura tudo com `GX_FOG_NONE`. Para ver o
  que ele muda, grave os mesmos frames com e sem e compare os BMPs; na rota
  o titulo, o menu e a SSS mudam e a luta em Hyrule Temple nao, porque a cena
  de luta nao instala fog. Se o shader e o rasterizador discordarem, rode a
  conformidade: metade dos casos usa um fog linear que cai no meio da curva
  na profundidade do quad.
- Uma cena que espera botao prende a rota para sempre, e o laco de modos so
  volta entre modos. `FRAME:STOP` encerra o roteiro naquele frame, com
  "stopped at frame N" e codigo 0, depois das outras entradas do frame; as
  linhas de resumo (`scenes:`, `route:`) nao saem, e o teste confere as
  linhas `scene 0xNN from frame N` que saem ao vivo. As conferencias de fim
  de rota tambem nao rodam: uma rota que pare cedo nunca falha por `MOVE` sem
  deslocamento ou por `FALLS` sem KO.
- O que a tela de resultados faz depois depende do save, que sem cartao
  comeca zerado: `FRAME:MATCHES[=TOTAL]` le e escreve o total de lutas VS (50
  e o menor que libera um personagem) e `FRAME:TROPHY=ID` da um trofeu pelo
  caminho do jogo (`fn_80172C78`), que deixa o aviso de premio pendente. O
  desafiante aparece para a porta do "smallest loser" da luta, que na rota de
  estoque e a do pad 2.
- Um valor que no console vem de um registrador some no host. Uma variavel
  sem atribuicao num caminho (`base` em `gm_80168B34`) valia o que o MWCC
  deixou no registrador que ela divide com um parametro, e uma funcao que
  termina sem `return` (`gm_80168BF8`) devolve o `r3` ou o `f1` da ultima
  chamada; no host sai o que estiver na pilha ou em outro registrador. O
  sintoma e uma escolha absurda: o frame de uma animacao de textura que troca
  nome, emblema ou retrato (o titulo "NO CONTEST" numa luta concluida, ruido
  nos retratos, o emblema errado no HUD). O GCC so faz a analise com
  otimizacao: um build `-O2` (`build/host-release`, com
  `-fno-strict-aliasing -fwrapv` e `MELEE_HOST_WARNINGS_AS_ERRORS=OFF`) lista
  os candidatos em `-Wmaybe-uninitialized` e `-Wreturn-type`, e acusa o
  `gm_80168B34` original. A maioria e falso positivo; antes de mudar, leia o
  codigo de maquina do DOL: tire os bytes da secao com o endereco, embrulhe
  com `llvm-objcopy -I binary -O elf32-powerpc` e desmonte com
  `llvm-objdump -d`.
- As rotas roteirizadas (`--run-modes`, `--run-title-scene`) congelam o
  relogio do OS em 3/12/2001 00:00:00. Congelado na hora do host, o relogio
  fazia a semente depender de quando a rota rodava: o titulo sorteia um
  `HSD_Rand` por segundo do minuto corrente, e a pose de vitoria dos
  resultados sai dessa semente. Duas execucoes simultaneas nao mostram isso,
  porque caem no mesmo segundo; compare execucoes em horas diferentes.
- `FIRST-LAST:TRACE=arquivo` grava por frame a cena, a semente e o estado de
  cada lutador, com floats em hex dos bits, e
  `port/tools/compare_match_trace.py A B` aponta o primeiro campo diferente
  (`--ignore campo` passa por uma diferenca ja entendida). Serve para comparar
  duas execucoes, o `host-debug` com o build `-O2`, ou uma rota com e sem
  `BMP=`.
- Para achar onde vai o tempo ha `gprof` (a maquina nao tem `perf` nem
  valgrind). Configure `build/host-profile` com `-O2 -g -pg
  -fno-strict-aliasing -fwrapv` nas flags de C e C++ e `-pg` no link, rode a
  rota numa pasta de trabalho (o `gmon.out` sai no diretorio corrente quando o
  processo termina) e leia `gprof -b -p melee-pc gmon.out`. Funcoes embutidas
  somam o tempo na que as chama: `finish_draw_locked` levava 92% da rota com
  o laco de `transform_captured_draw_locked` dentro, que refazia todos os
  triangulos do frame ao fim de cada draw.
- `port/src/gx/command_recorder.cpp` e `port/src/gx/tev.cpp` compilam com
  `-O2` em todos os presets, com `-g`: a captura GX roda por vertice e as
  copias da EFB por fragmento, e sem otimizacao a rota VS cancelada levava
  83,7 s (29,8 s assim). No gdb, variaveis desses dois arquivos podem aparecer
  como `<optimized out>`; para depurar um deles, tire a propriedade no
  `port/CMakeLists.txt` localmente.
- Sem perf, uma amostra de onde a rota gasta tempo sai de rodar sob
  `timeout -s INT N gdb -batch -ex run -ex "bt 12"` com alguns N diferentes;
  tres amostras bastaram para mostrar o TEV por fragmento das copias.
- Para medir o ritmo por cena, rode a rota com `stdbuf -oL`: num pipe o
  `stdout` sai em bloco no fim, e as linhas `scene 0xNN from frame N` chegam
  todas juntas.
- Para jogar, `melee-pc --play assets-local` abre uma janela e roda os modos a
  partir do titulo a 60 frames por segundo, com o relogio do OS na hora do
  host. Teclado como pad 1: WASD move o stick, as setas o C-stick e o teclado
  numerico (8, 4, 2, 6) o D-pad; J da o A, K o B, U o X, I o Y, Q o Z, H o L
  e L o R (apertados ate o fim, com o clique digital) e Enter o START; Esc ou
  fechar a janela encerra o processo. O titulo da janela mostra os frames por
  segundo apresentados, duas vezes por segundo, e o som sai no dispositivo
  padrao; o ritmo e o do campo NTSC, 59,94 frames por segundo. O primeiro gamepad substitui o teclado:
  D-pad, gatilhos que clicam no fim do curso, e o Back encerra. Um modo ou uma cena que o host nao tem, como o filme de
  abertura que segue o titulo parado, volta ao titulo. Entradas de roteiro
  valem por cima do pad, e `MELEE_HOST_PLAY_HIDDEN=1` desenha num presenter
  escondido, onde `BMP=` funciona: e assim que o `--play` e conferido sem abrir
  janela. O build `-O2` e o indicado; o `host-debug` roda a luta abaixo de
  60 Hz. O SDL transforma SIGTERM em fechar a janela.
- O som toca nas rotas e no `--play`: a cada retrace o relogio AX do host roda
  um quadro de 5 ms por 5 ms de campo, e `--run-modes` liga as vozes.
  `MELEE_HOST_AUDIO=0` desliga as vozes e o dispositivo do `--play`; o jogo da
  o mesmo trace com e sem som. `FIRST-LAST:WAV=arquivo` grava o que o mixer
  toca enquanto os frames desenhados estao no intervalo, 16 bits estereo a
  32 kHz; o cabecalho e escrito quando o intervalo termina, entao um processo
  parado depois (o menu principal parado nunca sai sozinho) deixa um WAV
  valido. `MELEE_HOST_AUDIO_AUX=0` deixa o reverb e o delay do jogo fora da
  mistura; e assim que o som de uma rota se compara com decodificadores de
  referencia, porque os efeitos enviam parte do som ao reverb.
- `port/tools/check_route_audio.py build/host-debug/port/melee-pc assets-local`
  roda titulo → menu → titulo gravando WAV e confere a musica `menu01.hps` e o
  efeito 118 de `main.ssm` contra decodificadores que nao passam pelo mixer:
  correlacao por janela a um atraso so, e o efeito com a musica subtraida. Uma
  amostra perdida ou repetida numa juncao de bloco do stream derruba as
  janelas seguintes.
- Para saber o que o jogo toca e quando, quebre no gdb em `AXDriver_8038E8EC`
  (caminho do `.hps`) e `HSD_Synth_80389334` (id do efeito), com
  `printf "%u", VIGetRetraceCount()` nos comandos do breakpoint.
- `port/tools/ssm_to_wav.py assets-local/audio/main.ssm --out dir` decodifica
  as vozes ADPCM de um banco de sons em WAV e mede pico, RMS, saturacao e passo
  medio. E a referencia do mixer AX do host: uma decodificacao errada aparece
  como saturacao em massa. Com `--compare-host build/host-debug/port/melee-pc`
  ele roda `melee-pc --decode-sound-bank` no mesmo banco e exige as mesmas
  amostras em cada voz.

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
- Um intrinseco do PowerPC trocado por macro em `src/placeholder.h` precisa
  dar o que a instrucao da, nao o que o nome lembra. `__frsqrte` e a
  estimativa de 1/sqrt(x) (`frsqrte`): os cerca de 50 lugares que o usam
  (`sqrtf_store`, `acosf` e `asinf` de `lbtrigf.c`, colisao, particulas,
  itens, dinamica) refinam com passos de Newton para 1/sqrt(x) e multiplicam
  por x. A macro devolvia `sqrt(x)`, que so converge perto de x = 1: em x = 2
  a raiz saia negativa e em 44 dava -1,9e41. O sintoma foi o IK das pernas
  (`lbBgFlash_80021410`, que `ft_80089B08` roda ao pousar e parado) com
  comprimentos absurdos e angulo NaN; a matriz da coxa e de tudo abaixo dela
  ficava NaN ate a animacao seguinte sujar o joint, e o envelope sumia com as
  pernas do Mario e do Link. Esconder DObjs e desligar a dinamica do chapeu
  nao tinham relacao com isso. Para achar quem grava um NaN numa matriz, arme
  no gdb um watchpoint de hardware em `mtx[0][0]` do joint com
  `gdb.Breakpoint(expr, gdb.BP_WATCHPOINT, gdb.WP_WRITE)` e um `stop()` que so
  para em `math.isnan`; o backtrace aponta a conta, e o `host-debug` mostra as
  variaveis locais. Uma rota com outro personagem sai do roteiro da Fox com
  `break Player_80031AD0 if slot == N` e `set player_slots[N].ckind =
  CKind_Link` nos comandos do breakpoint.
- Uma rota que precise de outro personagem nao depende mais do gdb: o cursor
  da CSS anda `(80*80 - 200) * 0,0002 = 1,24` unidade por frame com o stick em
  127 (o pad entrega 80), e as caixas dos icones estao em `mncharsel.c`. Com o
  save que a gravacao usa, a CSS mostra so os personagens desbloqueados em
  sete colunas. Partindo do roteiro da Fox, o pad 1 alcanca o Mario segurando
  cima por 15 frames em vez de 10 (a coluna e a mesma, uma linha acima) e o
  pad 2 alcanca o Link segurando direita por 30 frames na linha do meio; a
  ficha cai com A dois frames depois do ultimo frame de stick. `vs selection:`
  confirma a escolha (`0=8 1=6` para Mario e Link).
- Com `GXSetChanCtrl` desligando a iluminacao de um canal, o GX passa adiante
  so a cor de material: nem o registrador de ambiente nem as luzes entram. O
  avaliador do host partia do ambiente e multiplicava, entao todo draw sem
  iluminacao saia pintado pela cor ambiente que o ultimo material tivesse
  deixado no registrador. Hyrule Temple desenha o cenario assim: o estagio
  ficava escuro o tempo todo e vermelho enquanto o bumerangue do Link voava.
  Quando um desenho inteiro muda de tom sem que a geometria mude, compare a
  cor de raster dos vertices capturados (`captured_vertices`) entre dois
  frames antes de procurar luzes.
- Codigo decompilado que escreve num slot de pilha vizinho para casar com o
  MWCC (`*(&y + 6) = ...` em `it_802A4BFC_sqrtf_offset`, `itlinkhookshot.c`)
  corrompe o quadro de quem chama no host. O sintoma foi um SIGBUS com o
  backtrace destruido no `host-debug` e nenhum efeito visivel no `-O2`; o ASan
  aponta o objeto ("stack-buffer-overflow ... 'y'"). Guarde o truque com
  `#ifdef MELEE_HOST` e use a propria variavel; o ramo do console nao muda.

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
