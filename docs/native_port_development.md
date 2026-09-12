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
quantas vezes cada processo foi chamado. O processo do p_link pausado deve
terminar com zero chamadas.

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

A janela mostra a geometria que o caminho de display original desenhou, com as
texturas que o `GXLoadTexObj` original deixou ligadas, agrupada pelo estado de
pixel de cada draw. `--view-pobj` continua mostrando o que o schema de leitura
decodifica por conta propria, e serve de segunda opiniao quando as duas imagens
divergem.

Os comandos `--render-` listam os estados capturados, um por linha. Vale ler
essas linhas antes de culpar a geometria: `blend=1` com `zwrite=0` e o modo
translucido normal do HSD, `cull=0` marca um objeto de duas faces, e `alpha`
diferente de `7@0` significa que o material recorta por alpha.

A linha `material read exactly` diz quantos triangulos tiveram o programa TEV
lido de forma exata e quantos foram aproximados. Um numero alto de aproximados
nao e erro: significa que o material usa varios estagios, que o visualizador
ainda nao avalia. Ao mexer na reducao de TEV, olhe essa linha antes e depois,
porque e ela que mostra se a mudanca ampliou ou estreitou o que da para ler.

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
