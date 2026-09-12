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
