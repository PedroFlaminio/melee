# Plano de projeto: port nativo de Super Smash Bros. Melee para PC

Status: proposta inicial  
Base analisada: `doldecomp/melee`, commit `114e34ac5024211729b673baff28562143f910a0`  
Versao alvo inicial: `GALE01` (NTSC-U 1.02)  
Data da analise: 11 de setembro de 2026

## 1. Resumo executivo

O projeto deve produzir um executavel nativo, e nao empacotar o Dolphin nem
executar codigo PowerPC por emulacao. O codigo C de gameplay de Melee sera
compilado para a plataforma hospedeira, enquanto uma nova camada de plataforma
implementara os servicos que o jogo recebia do GameCube: GX, OS, VI, DVD, PAD,
CARD, AX, ARAM, DSP e THP.

O modelo de distribuicao sera semelhante ao Ship of Harkinian:

- o repositorio e os binarios distribuiveis nao conterao assets da Nintendo;
- na primeira execucao, o usuario fornecera uma copia legal compativel do disco;
- uma ferramenta verificara o dump e extraira/convertera os assets para um
  arquivo de recursos local;
- o executavel carregara codigo nativo e os recursos convertidos;
- recursos substitutos e mods poderao ser montados como camadas adicionais.

O caminho recomendado nao e tentar portar todos os subsistemas ao mesmo tempo.
Primeiro sera construida uma fatia vertical pequena: inicializacao, leitura de
assets, video, input e uma partida Fox vs. Fox em Final Destination. A partir
dela, os subsistemas serao expandidos ate cobrir todo o jogo.

Estimativa realista para uma versao 1.0:

- equipe principal de 8 a 12 pessoas: 18 a 30 meses;
- equipe principal de 3 a 5 pessoas: 30 a 48 meses;
- prova de conceito jogavel, sem qualidade de release: 4 a 8 meses.

Essas faixas pressupõem contribuidores experientes em C/C++, graficos e
engenharia reversa. O maior risco nao e mais decompilar gameplay; e reproduzir
corretamente a plataforma GameCube e eliminar dependencias do ABI PowerPC de
32 bits.

## 2. Estado da base e implicacoes

### 2.1 O que ja esta pronto

Na revisao analisada:

- os 1.118 objetos declarados em `configure.py` usam `Object(Matching, ...)`;
- ha aproximadamente 541 mil linhas de C em `src` e `extern/dolphin/src`;
- o ponto de entrada e o loop de jogo estao em C;
- gameplay, personagens, itens, estagios, menus e a `baselib` da HAL estao em C;
- o build matching continua capaz de reconstruir `main.dol` para PowerPC.

Isso permite usar o DOL original como oraculo de comportamento e criar testes
diferenciais muito precisos.

### 2.2 Por que a base ainda nao e portavel

O build atual foi feito para reproduzir o binario original, nao para obedecer a
um ABI moderno. Os bloqueadores observados incluem:

- `s32` e `u32` sao definidos com `long`, cujo tamanho muda em hosts LP64;
- estruturas assumem ponteiros de 4 bytes e possuem mais de 200 asserts de
  tamanho ou offset;
- o loader HSD reloca offsets gravando enderecos em slots de 32 bits;
- arquivos do disco e memory card sao big-endian;
- vertices imediatos sao escritos diretamente no FIFO GX em `0xCC008000`;
- inicializacao e loop chamam diretamente OS, VI, DVD, PAD, CARD e GX;
- ha 121 definicoes de funcoes `asm`, alem de quatro arquivos assembly;
- existem centenas de trechos condicionais `MUST_MATCH` e pragmas especificos
  do Metrowerks;
- audio depende de AX/DSP/ARAM e video depende de VI/GX/THP;
- callbacks assincronos de DVD, audio e retrace assumem a temporizacao e a
  concorrencia do console;
- diferencas de ponto flutuante podem alterar fisica, RNG derivado e
  determinismo de partidas.

Consequentemente, "100% decompilado" significa que o codigo C recompila para o
mesmo PowerPC; nao significa "100% pronto para x86-64 ou ARM64".

## 3. Definicao do produto

### 3.1 MVP

O MVP deve ser deliberadamente estreito:

- Windows 10/11 x86-64 e Linux x86-64;
- somente disco `GALE01` NTSC-U 1.02;
- extracao local de assets a partir do ISO/GCM fornecido pelo usuario;
- controles por teclado e gamepads SDL;
- video 16:9 ou 4:3, resolucao interna configuravel e tela cheia;
- simulacao fixa na cadencia original;
- apresentacao inicialmente sincronizada aos 60 Hz da simulacao, sem impedir
  uma futura camada de poses intermediarias;
- audio funcional com musica e efeitos;
- Versus local de dois a quatro jogadores;
- todos os 26 personagens e todos os estagios selecionaveis;
- save local e configuracoes atomicas;
- nenhum codigo ou asset proprietario distribuido nos releases.

### 3.2 Versao 1.0

A versao 1.0 acrescenta:

- Windows, Linux e macOS, em x86-64 e ARM64 onde aplicavel;
- todos os modos single-player e multiplayer local;
- cutscenes THP, trofeus, eventos, debug de compatibilidade e memory card;
- equivalencia visual, sonora e de gameplay validada por testes diferenciais;
- apresentacao de alta taxa de atualizacao, ate 240 Hz quando o hardware
  permitir, por interpolacao visual entre ticks de simulacao de 60 Hz;
- hotplug, rumble, remapeamento completo e perfis de controle;
- pacote de recursos versionado, verificavel e recompativel;
- API basica de mods e pacotes de assets substitutos;
- instalador/updater sem conteudo do jogo.

### 3.3 Fora do escopo inicial

- rollback netcode, matchmaking e compatibilidade com Slippi na build
  principal;
- mudancas de balanceamento ou mecanicas;
- aumentar a cadencia da simulacao acima dos 60 Hz originais;
- suporte a todas as revisoes e regioes do disco;
- Android, iOS, consoles e WebAssembly;
- editor visual de estagios/personagens;
- carregar ISOs sem extracao/conversao previa;
- substituir o formato de save original antes de existir compatibilidade.

Rollback deve orientar algumas decisoes desde o inicio — tempo deterministico,
input gravavel e estado serializavel — mas nao deve bloquear o primeiro release.

### 3.4 Alta taxa de atualizacao e build Slippi futura

A simulacao do Melee permanece fixa em 60 Hz. Ela e a fonte de verdade para
fisica, inputs, frame data, timers e determinismo. A apresentacao pode ser
independente: o renderer guardara os estados anterior e atual e, entre dois
ticks, amostrara poses intermediarias de camera, esqueletos, transformacoes e
outros dados visuais seguros. A opcao de apresentacao oferecera somente os
limites 60, 120, 144, 165 e 240 FPS; nao havera modo ilimitado. A meta de
produto e atingir o limite selecionado sem executar logica extra nem alterar o
resultado de uma partida.

Essa camada sera desenvolvida e validada depois da paridade visual basica a 60
Hz. Cortes de camera, teleporte, spawn, troca de cena, efeitos sem estado
interpolavel e qualquer descontinuidade devem preservar o quadro valido, nunca
inventar uma posicao que afete a simulacao.

Compatibilidade com Slippi nao e requisito da primeira versao. Se for adotada
posteriormente, ela sera entregue em uma build especifica, opcional e separada
da build principal. Essa build devera manter a simulacao estritamente
deterministica, implementar a interface/protocolo esperado pelo Slippi e ser
validada contra o Slippi Dolphin; recursos visuais de alta taxa permanecem
somente no renderer e nao entram no estado sincronizado.

## 4. Arquitetura proposta

```text
                  +-----------------------------+
ISO/GCM do usuario| verificador + extrator      |
----------------->| FST, HSD, texturas, audio   |
                  +-------------+---------------+
                                |
                                v
                  +-----------------------------+
                  | melee.pak + manifest.json   |
                  | recursos big-endian tratados|
                  +-------------+---------------+
                                |
                                v
+-------------------+   +-------+---------+   +--------------------+
| Codigo C de Melee |-->| libmelee_host   |-->| SDL3 / sistema     |
| gameplay + baselib|   | C ABI compativel|   | janela/input/files |
+-------------------+   +---+---+---+-----+   +--------------------+
                          |   |   |
                    +-----+   |   +----------------+
                    v         v                    v
              +----------+ +----------+      +-----------+
              | GX host  | | AX host  |      | DVD/CARD  |
              | renderer | | audio    |      | recursos  |
              +----+-----+ +----+-----+      +-----------+
                   |            |
                   v            v
             wgpu-native   SDL audio graph
             D3D12/Vulkan/
             Metal
```

### 4.1 Principios

1. Preservar o build matching original como referencia permanente.
2. Manter alteracoes de port sob `MELEE_HOST` e interfaces pequenas, evitando
   poluir gameplay com `#ifdef` por plataforma.
3. Usar C ABI no limite entre o jogo e a camada host; C++ pode ser usado por
   baixo para recursos, renderer, UI e ferramentas.
4. Decodificar dados serializados em estruturas runtime nativas. Nao tratar um
   blob big-endian com ponteiros de 32 bits como uma struct C de 64 bits.
5. Manter simulacao e apresentacao separadas. Resolucao, widescreen e FPS de
   apresentacao nao podem mudar a logica da luta.
6. Priorizar testes diferenciais contra o DOL sobre "parece correto".
7. Nao acoplar o projeto a um backend grafico ou sistema operacional.

### 4.2 Organizacao sugerida do repositorio

```text
src/                         # decomp original, mantida sincronizavel
extern/dolphin/              # headers/API original
port/
  include/melee_host/        # interfaces C estaveis
  src/os/                    # tempo, threads, alarmes, filas, memoria
  src/io/                    # DVD virtual, arquivos, CARD
  src/input/                 # PAD, teclado, gamepads, rumble
  src/gx/                    # estado GX, TEV, shaders e draw submission
  src/audio/                 # AX, DSP ADPCM, mixer, streaming
  src/video/                 # janela, VI, frame pacing, THP
  src/assets/                # runtime resource manager
  src/ui/                    # configuracao e diagnostico
tools/
  extractor/                 # ISO/GCM -> melee.pak
  trace/                     # captura e comparacao com Dolphin
tests/
  unit/
  differential/
  replay/
cmake/
CMakeLists.txt               # build host; build matching permanece separado
```

## 5. Decisoes tecnicas

### 5.1 Build e linguagem

- CMake + Ninja para o build nativo.
- C17 para o codigo C e C++20 para a camada host.
- Clang e MSVC como compiladores suportados; GCC em CI como verificador extra.
- SDL3 para janela, eventos, gamepads, rumble e abstracoes basicas de audio.
- `wgpu-native` como primeira opcao de renderer, expondo D3D12, Vulkan e Metal.
- ImGui somente para configuracao e ferramentas; nunca para UI original do jogo.
- Sanitizers, warnings altos e analisadores estaticos ligados no codigo do port.

O renderer deve ficar atras de uma interface propria. Se a prova de conceito
mostrar que a geracao dinamica de shaders TEV e inadequada em WebGPU, sera
possivel trocar internamente por Vulkan/D3D/Metal ou bgfx sem alterar o jogo.

### 5.2 Estrategia de 32 para 64 bits

Nao e recomendado publicar um executavel de 32 bits como arquitetura final. A
transicao deve ocorrer em duas pistas:

- uma build de bootstrap x86 de 32 bits, temporaria, pode acelerar o primeiro
  boot e ajudar a localizar problemas que nao sao de ABI;
- a build de produto deve ser 64-bit clean desde o primeiro trimestre.

Passos obrigatorios:

1. substituir aliases primitivos por `stdint.h` no modo host;
2. classificar cada cast ponteiro-inteiro como endereco, offset, ID ou flags;
3. criar tipos explicitos como `HsdOffset32`, `AssetId` e `RuntimeHandle`;
4. separar estruturas `*Disk`/`*BE` das estruturas runtime;
5. trocar relocacao in-place de HSD por desserializacao e pointer swizzling;
6. manter tabelas para referencias ciclicas e externas;
7. rodar ASan/UBSan e testes em x86-64 e ARM64 desde cedo.

O loader HSD e o sistema de particulas formam o primeiro caso de teste, pois
ambos gravam enderecos diretamente sobre offsets de 32 bits.

### 5.3 Recursos e distribuicao

A ferramenta `melee-extract` deve:

1. aceitar ISO/GCM ou diretorio previamente extraido;
2. identificar revisao/regiao por hash e metadados do disco;
3. recusar silenciosamente nada: erros devem explicar o arquivo esperado;
4. ler FST e extrair somente os arquivos necessarios;
5. validar tamanho/hash dos arquivos essenciais;
6. converter metadados big-endian para um formato versionado;
7. preservar dados comprimidos quando nao houver beneficio em expandi-los;
8. produzir `melee.pak` e um manifesto sem depender do caminho do ISO;
9. permitir rebuild incremental e verificacao de integridade;
10. jamais enviar o ISO ou arquivos extraidos a um servidor.

O resource manager montara, em ordem: `melee.pak`, patches oficiais do port,
mods do usuario e overrides soltos de desenvolvimento. Cada recurso tera tipo,
versao de schema, hash, dependencias e nome logico.

### 5.4 Camada OS e memoria

Implementar somente a superficie realmente usada pelo jogo:

- arena e heaps sobre allocators host;
- alarmes, ticks e calendario;
- mutexes, filas de mensagens e threads;
- interrupcoes como secao critica/reentrancia, e nao como emulacao de CPU;
- cache flush/invalidate como no-op validado ou barreira quando necessario;
- logs, asserts, panic e crash report;
- callbacks assincronos entregues em pontos deterministas do game thread.

MetroTRK, suporte a hardware EXI de debug, registradores PPC e boot ROM nao
entram no executavel host. Stubs devem declarar explicitamente se a operacao e
no-op segura, nao suportada ou erro fatal.

### 5.5 GX e renderer

O renderer e a maior frente isolada do projeto. A camada `gx_host` deve modelar
o estado observado pela API GX, nao emular o chip Flipper ciclo a ciclo.

Componentes:

- substituicao das escritas `GXWGFifo` por um command encoder no modo host;
- vertex descriptors, formatos, arrays indexados e display lists;
- matrizes de posicao, normal e textura;
- texturas GameCube, TLUT, mipmaps, wrap e filtros;
- blending, depth, alpha compare, culling, fog, scissor e viewport;
- copy EFB/XFB, screenshots, shadows e efeitos que leem framebuffer;
- compilador TEV: estado GX -> IR canonica -> shader;
- cache persistente de pipelines/shaders por chave de estado;
- ubershader de fallback para evitar travadas durante compilacao;
- marcadores e captura de frame para RenderDoc;
- caminho de referencia por software para pequenos testes de TEV.

Ordem de implementacao:

1. clear, viewport, triangulos sem textura;
2. vertex arrays e matrizes;
3. textura simples e blending;
4. TEV de um estagio;
5. multiplos estagios, indirect texturing e fog;
6. EFB copies, shadows e casos especiais;
7. cache, performance, widescreen e resolucao interna.

Usar codigo do Dolphin diretamente so deve ocorrer apos decisao explicita de
licenca. A arquitetura do emulador e mais ampla do que o necessario e sua
licenca pode determinar a licenca de toda a camada derivada. Uma implementacao
limpa das APIs usadas reduz acoplamento, mas exige testes fortes.

### 5.6 Audio, ARAM e DSP

A camada de audio deve preservar o modelo de vozes AX sem emular o DSP:

- parser e decoder de DSP ADPCM;
- vozes, prioridade, pitch, volume, pan, envelopes e looping;
- aux sends e efeitos usados pela HSD Synth;
- streaming de musica com buffer assincrono;
- ARAM virtual como armazenamento/handles, nao endereco fisico;
- mixer em ponto flutuante com conversao final para o dispositivo SDL;
- resampling independente da taxa do dispositivo;
- log de eventos de audio para comparacao deterministica.

Primeiro objetivo: musica e SFX audiveis na fatia vertical. Fidelidade de mix,
reverb e casos extremos vem depois; latencia baixa e ausencia de underruns sao
criterios de release.

### 5.7 VI, tempo e frame pacing

- simulacao em tick fixo, derivado da temporizacao NTSC original;
- `VIGetRetraceCount` e callbacks modelados pelo scheduler host;
- render desacoplado, sem executar dois ticks por engano em monitores de 120 Hz;
- opcao de interpolation somente para transformacoes visuais validadas;
- pausa quando a janela perde foco configuravel;
- medicao de input-to-photon no modo de diagnostico;
- nenhum `sleep` deve ser fonte da verdade da simulacao.

### 5.8 Input e rumble

- PAD 0-3 sobre SDL Gamepad;
- deadzones, calibration, trigger analogico e gate octogonal configuraveis;
- teclado como dispositivo equivalente;
- hotplug sem alterar indices durante uma partida;
- rumble com fallback quando o dispositivo nao o suporta;
- captura/replay de input por tick como formato de teste estavel;
- adaptadores GameCube tratados primeiro como HID/SDL, com backend especializado
  opcional se a latencia justificar.

### 5.9 DVD, CARD e saves

- mapear paths e entry numbers do DVD para o resource manager;
- manter semantica assincrona e ordem dos callbacks;
- implementar um memory card virtual compatível com os dados de Melee;
- gravacoes com arquivo temporario, `fsync` apropriado e rename atomico;
- backups rotativos e recuperacao apos interrupcao;
- importar/exportar GCI quando tecnicamente validado;
- separar configuracao do port do save do jogo.

### 5.10 THP e cutscenes

Implementar o container THP e decodificar video/audio com bibliotecas auditadas,
ou adaptar o decoder C existente apos remover otimizacoes PPC. Sincronizacao
audio-video, seek usado pelo jogo e conversao de cores devem ter testes proprios.

### 5.11 Determinismo e ponto flutuante

O projeto nao deve prometer determinismo antes de medi-lo. A estrategia sera:

- builds de referencia com FMA/FP contraction controlados;
- implementacoes conhecidas para estimativas PPC como `frsqrte`, quando afetam
  gameplay;
- auditoria de undefined behavior, casts, shifts e aliasing;
- estado de RNG e input incluidos em toda trace;
- hashes por tick apenas sobre estado logico canonico, sem ponteiros;
- comparacoes com tolerancia somente onde a diferenca nao realimenta gameplay;
- teste de partidas longas e rollback/snapshot mesmo antes do netplay.

## 6. Estrategia de verificacao

### 6.1 Oraculo de referencia

Executar o DOL correspondente no Dolphin instrumentado e o port nativo com a
mesma sequencia de inputs. O mapa de simbolos da decompilacao permite capturar
estado sem inferir enderecos manualmente.

Por tick, registrar:

- cena e estado da maquina de jogo;
- seed/RNG;
- action state, posicao, velocidade, dano e stocks de cada fighter;
- entidades, itens e resultados de colisao relevantes;
- eventos de audio;
- assinatura canonica dos comandos GX;
- hash de estado serializado.

Por marcos visuais, comparar:

- screenshot com mascara para elementos temporais;
- profundidade quando necessaria;
- pipeline/TEV esperado;
- tolerancia perceptual e mapa de diferenca.

### 6.2 Piramide de testes

- unitarios: endian, FST, HSD relocation, ADPCM, TEV IR, CARD e matematicas;
- contrato: cada funcao host imita casos observados da API Dolphin;
- replay: sequencias curtas deterministicas por personagem/estagio;
- diferencial: port versus DOL no Dolphin;
- visual: golden images por backend/GPU com tolerancia definida;
- soak: partidas automatizadas de 8 a 24 horas;
- fuzz: parsers de disco, HSD, THP e save;
- performance: tempo de tick, frame, shader compilation, audio e memoria.

### 6.3 CI

Matriz minima:

- Windows x86-64: MSVC e Clang;
- Linux x86-64: Clang e GCC;
- macOS ARM64: Clang a partir do milestone de 1.0;
- ASan/UBSan no Linux;
- build matching PowerPC para impedir regressao na base;
- build sem assets sempre deve concluir;
- testes que precisam de assets rodam apenas em workers privados, com hashes e
  resultados publicos, nunca publicando os arquivos.

## 7. Roadmap e gates

### Fase 0 — charter, licenca e baseline (semanas 1-4)

Entregas:

- charter de escopo e governanca;
- decisao de licenca para codigo novo e politica de contribuicao;
- estrategia de sincronizacao com `doldecomp/melee`;
- inventario gerado de APIs de plataforma, asm, casts e layouts;
- corpus inicial de replays/traces no DOL;
- CI do build matching.

Gate: qualquer commit do port preserva a reconstrucao matching e existe uma
politica clara de assets/licencas.

### Fase 1 — build host e ABI (meses 1-3)

Entregas:

- CMake compila o codigo relevante em x86-64 com stubs;
- tipos inteiros fixos e headers libc host;
- exclusao explicita de Runtime/MetroTRK/hardware;
- camada C ABI `melee_host`;
- ASan/UBSan e relatorio dos casts perigosos;
- executable skeleton chega ao `main` e encerra de forma controlada.

Gate: build host sem assembly PPC e sem acesso a enderecos MMIO.

### Fase 2 — recursos, OS e boot headless (meses 2-5)

Entregas:

- verificador/extrator de `GALE01`;
- DVD virtual e loader HSD 64-bit/big-endian;
- memoria, tempo, filas, alarmes e jobs assincronos;
- renderer e audio nulos que capturam comandos;
- boot headless ate a primeira cena/menu.

Gate: mesma sequencia de cenas e mesmo RNG inicial do DOL em um replay de boot.

### Fase 3 — renderer GX minimo (meses 3-8)

Entregas:

- janela, VI e frame pacing;
- GX command encoder, vertices, matrizes, texturas e TEV essencial;
- title screen, menus e Final Destination renderizados;
- captura visual automatizada.

Gate: fatia vertical visual executa sem erros de validacao da API grafica.

### Fase 4 — fatia vertical jogavel (meses 5-10)

Entregas:

- PAD/rumble;
- AX audio minimo;
- partida Fox vs. Fox, quatro controles e HUD;
- pause, fim de partida e retorno ao menu;
- replay diferencial de cinco minutos.

Gate: gameplay nao diverge do DOL durante o replay acordado e o frame budget e
mantido em hardware de referencia.

### Fase 5 — cobertura de conteudo (meses 8-16)

Entregas:

- todos os personagens, itens e estagios;
- single-player, eventos, trofeus e menus restantes;
- GX avancado, particulas, framebuffer effects e shadows;
- THP, audio completo, CARD e saves;
- suite de replays por matriz personagem/estagio.

Gate: checklist de conteudo completo, zero crash conhecido de severidade alta e
traces de gameplay dentro da politica de equivalencia.

### Fase 6 — portabilidade, fidelidade e performance (meses 13-22)

Entregas:

- macOS/ARM64 e multiplos backends;
- cache de shaders sem stutter recorrente;
- profiling e reducao de latencia;
- 4:3, widescreen seguro e resolucao interna;
- acessibilidade basica, remapeamento e configuracao;
- soak, fuzz e compatibilidade ampla de GPU/controle.

Gate: metas de desempenho e compatibilidade atendidas na matriz de hardware.

### Fase 7 — release 1.0 (meses 20-30)

Entregas:

- instalador, updater e crash diagnostics opt-in;
- fluxo de extracao compreensivel para usuario final;
- documentacao de build, uso e troubleshooting;
- auditoria de licencas e de ausencia de assets;
- beta publica, triagem e release candidates reproduziveis.

Gate: criterios da secao 8 atendidos por dois release candidates consecutivos.

Fases se sobrepoem por equipe. As datas sao faixas de planejamento, nao promessa
de calendario.

## 8. Criterios de aceite da versao 1.0

Funcionalidade:

- o usuario consegue gerar os recursos a partir de um dump suportado;
- todos os modos acessiveis no DOL alvo podem ser concluidos;
- saves sobrevivem a encerramento inesperado durante operacoes nao criticas;
- quatro controles, hotplug e rumble funcionam;
- cutscenes, musica e SFX permanecem sincronizados.

Fidelidade:

- corpus competitivo de replays nao apresenta divergencia logica nao explicada;
- diferencas de imagem ficam dentro dos limites aprovados por cena;
- nenhuma melhoria altera fisica/timing quando o modo de compatibilidade esta
  ativo;
- RNG e ordem de callbacks sao reproduziveis com mesmo input/configuracao.

Performance:

- tick de simulacao abaixo de 4 ms no hardware minimo definido;
- frame dentro do budget a 1080p no hardware minimo;
- sem underruns persistentes de audio;
- sem crescimento de memoria em soak test de 8 horas;
- stutter de shader fica restrito ao primeiro uso ou e absorvido pelo fallback.

Qualidade e distribuicao:

- zero defeito aberto critico e zero corrupcao de save conhecida;
- builds reproduziveis e assinados para plataformas suportadas;
- pacote publico nao contem assets do jogo;
- SBOM e avisos de terceiros acompanham o release;
- crash reporter e telemetria sao opt-in.

## 9. Organizacao da equipe

Frentes que podem trabalhar em paralelo:

- arquitetura/build/ABI: 2 pessoas;
- GX/renderer: 3 a 4 pessoas;
- OS, input, DVD e CARD: 2 pessoas;
- assets e ferramentas: 1 a 2 pessoas;
- audio/THP: 2 pessoas;
- determinismo, testes e CI: 2 pessoas;
- release, UX e documentacao: 1 pessoa, crescendo perto da beta.

Algumas pessoas podem ocupar mais de uma frente, mas renderer, audio e testes
diferenciais precisam de responsaveis claros. Cada subsistema tera owner,
backup, interface documentada e dashboard de cobertura.

Cadencia sugerida:

- roadmap trimestral por gates, nao por numero de funcoes portadas;
- demos jogaveis quinzenais;
- RFC obrigatoria para formato de assets, GX IR, ABI e save;
- bugs de divergencia recebem replay minimo antes do conserto;
- toda melhoria opcional deve poder ser desligada pelo modo de compatibilidade.

## 10. Riscos prioritarios

| Risco | Impacto | Mitigacao |
|---|---:|---|
| Pointer swizzling HSD em 64 bits | Critico | Estruturas Disk/Runtime, IDs e testes de grafos ciclicos |
| Semantica TEV/EFB incompleta | Critico | IR canonica, renderer de referencia e comparacao com Dolphin |
| Divergencia de ponto flutuante | Critico | Traces por tick, funcoes PPC controladas e corpus competitivo |
| AX/DSP com mix incorreto | Alto | Log de vozes, decoder testado e comparacao offline |
| Callbacks assincronos mudam ordem | Alto | Scheduler deterministico e entrega no game thread |
| Stutter por shaders | Alto | Cache persistente, prewarm e ubershader fallback |
| Corrupcao de save | Alto | Escrita atomica, backup e fuzz de interrupcao |
| Fork diverge da decomp | Alto | Build dual, upstream merges automatizados e poucos ifdefs |
| Licenca de codigo reaproveitado | Alto | RFC/licence review antes de copiar Dolphin ou outro port |
| Escopo cresce para netplay/mods | Alto | MVP congelado e epics posteriores separados |
| Assets entram em CI/release | Critico | workers privados, scanner de artefatos e manifestos por hash |

## 11. Primeiros 90 dias

### Dias 1-30

- aprovar charter, licenca e estrutura do fork;
- automatizar inventario de dependencias GameCube;
- definir interfaces `host_os`, `host_gx`, `host_audio`, `host_io` e `host_pad`;
- preparar cinco replays de referencia no Dolphin;
- criar build CMake vazio e CI multiplataforma;
- iniciar conversao de tipos fixos sem quebrar matching.

### Dias 31-60

- compilar `lb`, `gm` e partes de `sysdolphin` no host;
- substituir MSL/Runtime e assembly PPC por implementacoes host ou exclusao;
- implementar FST/ISO, manifesto e leitura DVD sincrona;
- prototipar parser HSD big-endian com swizzling 64-bit;
- criar OSReport/panic, clock e allocator;
- fazer um programa de teste carregar e inspecionar um modelo do disco.

### Dias 61-90

- ligar o entry point a stubs auditaveis;
- chegar ao boot headless com command logs;
- abrir janela SDL e limpar framebuffer;
- mapear PAD e gravar/reproduzir input por tick;
- implementar primeiro triangulo via subset GX;
- publicar relatorio de riscos atualizado e estimativa do vertical slice.

Resultado esperado ao fim de 90 dias: nao um jogo completo, mas uma demonstracao
que reduz os tres maiores riscos — build host, assets 64-bit e caminho GX — e
fornece dados suficientes para confirmar ou revisar o cronograma.

## 12. Indicadores de progresso

Evitar usar "percentual de codigo compilado" como indicador principal. Medir:

- numero de APIs host chamadas versus implementadas e validadas;
- cenas que completam boot e transicao;
- combinacoes personagem/estagio cobertas por replay;
- ticks consecutivos sem divergencia logica;
- estados GX/TEV cobertos e pipelines de fallback usados;
- formatos de asset decodificados;
- modos de jogo concluidos;
- GPUs, sistemas e controles aprovados;
- defeitos de fidelidade, crashes e corrupcao por severidade;
- p95/p99 de tick, frame, audio callback e shader compilation.

O dashboard deve distinguir `stub`, `funcional`, `equivalente` e `otimizado`.
Uma API que apenas retorna zero nao conta como concluida.

## 13. Decisao de inicio (go/no-go)

O projeto recebe sinal verde para desenvolvimento completo quando a prova de
conceito demonstrar simultaneamente:

1. um asset HSD real carregado corretamente em x86-64;
2. um frame real emitido pela baselib atraves do subset GX host;
3. boot deterministico repetivel ate uma cena conhecida;
4. ausencia de dependencia inevitavel de emulacao PowerPC;
5. caminho de licenca e distribuicao aprovado.

Se o subset GX revelar custo desproporcional, o projeto deve revisar renderer,
bibliotecas e licenca — nao trocar silenciosamente o objetivo por empacotar um
emulador. Se o swizzling 64-bit for inviavel no prazo, uma build x86 de 32 bits
pode continuar servindo como ferramenta de pesquisa, mas nao substitui a meta
de produto multiplataforma.

## Referencias de arquitetura

- Decompilacao Melee: <https://github.com/doldecomp/melee>
- Shipwright / Ship of Harkinian: <https://github.com/HarbourMasters/Shipwright>
- Instrucoes de build do Shipwright:
  <https://github.com/HarbourMasters/Shipwright/blob/develop/docs/BUILDING.md>
