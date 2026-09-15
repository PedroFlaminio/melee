# Melee Unlocked: referências para acelerar o port nativo

Levantamento em 15/09/2026, a partir dos dois checkouts locais.

| Base examinada | Revisão |
| --- | --- |
| `melee-unlocked/` | `6e2d56a6351e95b7a7d6d487a29f0cbd4e6b9643` — `Make settings closed state passive` |
| Nosso `melee/` | `41816018339db6e05f0e02767ee11562424a6c81` — mais as alterações locais já existentes |

## Resumo e recomendação

O melhor aproveitamento imediato é usar o Melee Unlocked como referência de
comportamento, fonte de testes e exemplo de implementação dos serviços do
console. Os maiores ganhos potenciais estão em áudio, validação de gameplay e
renderização GX. Adotar sua recompilação estática como base seria uma mudança
de arquitetura, com custo próprio de integração e suporte a Linux.

Para o estágio atual do nosso port, recomendo esta ordem:

1. Consolidar a luta Fox/Fox até vitória por estoque e usar seus estados como
   base de comparação reproduzível. O teste dessa rota **já existe no worktree**.
2. Completar a execução de vozes AX e produzir áudio verificável.
3. Corrigir diferenças gráficas com capturas e estados GX equivalentes.
4. Ampliar personagens/estágios e persistir saves.
5. Tratar alta taxa de atualização e online depois de estabelecer essa base.

Este documento distingue observação de código, resultados executados aqui e
afirmações da documentação do projeto. Não foi executada uma partida do
Melee Unlocked, nem feita uma verificação de compatibilidade com Slippi.

## 1. A diferença de arquitetura muda o que podemos aproveitar

| Aspecto | Melee Unlocked | Nosso port | Consequência prática |
| --- | --- | --- | --- |
| Código do jogo | Traduz o DOL PowerPC para C++; inclui códigos Gecko/Slippi por padrão | Compila o C da decompilação com `MELEE_HOST` | O código gerado não substitui diretamente nossos módulos C |
| Memória | RAM convidada de 24 MiB, endereços de 32 bits e leituras/escritas big-endian | Objetos e ponteiros nativos de 64 bits, descritores materializados | Os tradutores de assets continuam necessários aqui |
| SDK do console | HLE: funções host recebem registradores de uma CPU PowerPC representada em software | Fachadas com ABI C e tipos adaptados ao host | Reaproveitar a lógica exige trocar a fronteira de chamadas |
| Vídeo | Estado GX por registradores, shaders HLSL, D3D12 | Recorder da API GX, shaders GLSL, SDL/OpenGL | Fórmulas e casos de teste são mais transferíveis que o backend |
| Plataforma do executável | Windows x64/MSVC, opções AVX2 e serviços Win32 | Desenvolvimento atual em Linux | Não é uma biblioteca pronta para ligar ao nosso executável |
| Rollback | Copia regiões da memória convidada com exclusões específicas | Estado espalhado por objetos e alocações nativas | Savestates precisam de uma estratégia própria |

Fontes: [CMake do Unlocked](../../melee-unlocked/port/CMakeLists.txt),
[contexto e memória PPC](../../melee-unlocked/port/runtime/ppc/ppc.h),
[ABI HLE](../../melee-unlocked/port/runtime/hle/hle.h),
[nosso CMake](../port/CMakeLists.txt) e
[materialização HSD](../port/src/assets/hsd_materialize.cpp).

O Unlocked preserva o layout de memória do console. Por isso, a execução de um
personagem nele não demonstra que exista um schema reutilizável de `ftData*`
para 64 bits. No nosso código, o registro específico de personagem continua
sendo `ftDataFox`, em
[game_data_translators.c](../port/src/game/game_data_translators.c).

Há uma opção útil para comparação com o jogo sem modificações:
`port/recomp/recomp.py --no-slippi`. Ela desativa a inclusão das tabelas Slippi
na tradução. Sua existência foi conferida; uma build vanilla não foi executada.
Não comparar diretamente nosso comportamento vanilla com uma build modificada
sem alinhar códigos, regras, saves, RNG e entradas.

## 2. Ponto de partida real do nosso port

O histórico de [port-mvp-progress.md](port-mvp-progress.md) registra Fox/Fox em
Hyrule Temple, movimento, ataque, blaster, KO/renascimento em sondagem e a rota
de resultados após cancelamento. A introdução desse documento ainda apresenta
uma estimativa antiga de 40%, enquanto o histórico chega a 87%; esses números
não são uma medição nova deste levantamento.

O worktree está à frente de partes da documentação:

- [port/CMakeLists.txt](../port/CMakeLists.txt) já define
  `melee-host-vs-stock-match-asset`: altera as regras pelo menu, escolhe uma vida,
  provoca a queda de P1 e exige resultado com P2 vencedor, estoques `0/1` e volta
  à seleção de personagens.
- [match_trace.c](../port/src/game/match_trace.c) já expõe posição, ação, quedas,
  regras e resultado; [main.cpp](../port/src/main.cpp) consome eventos
  `FALLS`, `RULES` e `RESULT` no roteiro.
- [baselib_support.c](../port/src/os/baselib_support.c) ainda retorna `NULL` em
  `AXAcquireVoice`: o synth inicializa, mas nenhuma voz é disponibilizada.
- O renderer já tem geração/cache de shaders TEV; fog e estado indireto
  registrados na camada GX não significam que esses efeitos sejam aplicados
  pelo shader apresentado.

Portanto, o próximo passo para o KO é verificar e consolidar o teste existente.
Não foi reexecutada a suíte do nosso port nesta tarefa; a presença do teste não
é tratada como evidência de aprovação. As seis alterações locais preexistentes
foram preservadas.

## 3. Mapa de referências por retorno esperado

Os custos abaixo são estimativas relativas de adaptação, não prazos.

| Prioridade | Referência no Unlocked | Aplicação aqui | Custo relativo |
| --- | --- | --- | --- |
| P0 | [`validate_native.py`](../../melee-unlocked/tools/validate_native.py), [`replay_compare.py`](../../melee-unlocked/tools/replay_compare.py) | Comparar estados da luta e localizar o primeiro frame divergente | Baixo/médio para o método; alto para equivalência completa |
| P0 | [`ax_ucode.cpp`](../../melee-unlocked/port/runtime/hle/ax_ucode.cpp), [`ax_ucode_test.cpp`](../../melee-unlocked/port/tests/ax_ucode_test.cpp) | Decoder/mixer AX, escrita de estado das vozes e testes com amostras conhecidas | Médio/alto |
| P1 | [`gx_shader.cpp`](../../melee-unlocked/port/runtime/gx/gx_shader.cpp), [`gx_regs.h`](../../melee-unlocked/port/runtime/gx/gx_regs.h) | TEV indireto, fog, swaps e profundidade | Médio, por efeito |
| P1 | [`texture_snapshot.h`](../../melee-unlocked/port/runtime/gx/texture_snapshot.h), [`texture_snapshot_test.cpp`](../../melee-unlocked/port/tests/texture_snapshot_test.cpp) | Garantir que cada draw retenha sua textura e paleta | Baixo para adaptar os testes |
| P1 | [`hle_card.cpp`](../../melee-unlocked/port/runtime/hle/hle_card.cpp) | Saves em pasta GCI e contrato da API CARD | Médio |
| P1 | [`hle_dvd.cpp`](../../melee-unlocked/port/runtime/hle/hle_dvd.cpp) | I/O em worker com entrega determinística | Médio; nossa fila já existe |
| P2 | [`ppc.h`](../../melee-unlocked/port/runtime/ppc/ppc.h), [`ppc_runtime.cpp`](../../melee-unlocked/port/runtime/ppc/ppc_runtime.cpp) | Referência para diferenças de ponto flutuante | Alto para equivalência total |
| P2 | [`authored_pose.cpp`](../../melee-unlocked/port/runtime/gx/authored_pose.cpp), [`subframe.cpp`](../../melee-unlocked/port/runtime/gx/subframe.cpp) | Apresentação acima de 60 Hz sem acelerar gameplay | Alto |
| P3 | [`slippi_online.cpp`](../../melee-unlocked/port/runtime/hle/slippi_online.cpp), [`slippi_net.cpp`](../../melee-unlocked/port/runtime/hle/slippi_net.cpp) | Estudar protocolo, snapshots e sincronização | Muito alto no nosso layout nativo |

## 4. Validação: provavelmente o ganho mais rápido

### 4.1 Separar três perguntas

| Pergunta | Técnica encontrada | Limite |
| --- | --- | --- |
| O renderer altera a simulação? | `validate_native.py` compara checkpoints em headless, janela escondida, renderer em thread e modo authored | Compara o Unlocked consigo mesmo |
| O jogo reproduz a referência? | `replay_compare.py` compara estados de uma gravação `.slp` com a nova execução | Só observa parte do estado e tem lacunas de cobertura |
| Frames extras têm imagem diferente? | [`diff_captures.py`](../../melee-unlocked/tools/diff_captures.py) conta pixels diferentes entre capturas PPM | Diferença visual não demonstra pose correta ou menor latência |

O `validate_native.py` exige a quantidade solicitada de checkpoints e rejeita
diagnósticos de MMIO inválido/FATAL. O runtime gera hashes de CPU, RAM e ARAM;
o campo `events` resume parte do estado de eventos, não serializa toda a fila.
O próprio script afirma que não verifica equivalência com Dolphin nem estado
completo de rollback.

### 4.2 Lacunas concretas do comparador de replays

Na revisão examinada, `replay_compare.py`:

- Compara `state`, `x`, `y`, `facing`, `percent`, `stocks` e `char`, por
  jogador/follower. Embora leia `shield`, não o inclui na comparação final.
- Usa a **interseção** dos frames: um replay incompleto pode passar se os frames
  em comum forem iguais. Não exige a mesma cobertura temporal.
- Lê configurações e entradas, mas não as compara no laço final.
- Compara floats como números Python, não como representações binárias.
- Imprime o código de saída do executável, mas não o usa diretamente como
  condição de reprovação se ainda encontrar uma gravação para comparar.

Ao adaptar, exigir cobertura de frames/jogadores, configuração compatível e
execução bem-sucedida. Usar comparação binária quando a meta for determinismo
bit a bit; tolerância numérica só serve a um critério explicitamente aproximado.
O parser atual também não deve ser presumido um leitor genérico de toda versão
de `.slp`.

### 4.3 Aplicação proposta no nosso código

Expandir a fachada já existente de `match_trace` para emitir, por tick, um
registro canônico: cena, RNG, entradas, ação, posição, velocidade, dano,
estoques, flags relevantes e resultado. Usar slots/IDs estáveis para objetos.
Não hashear structs nativas inteiras: padding e endereços de 64 bits introduzem
diferenças que não são diferenças de gameplay.

Primeiro comparar duas execuções nossas com o mesmo roteiro e assets. Depois
comparar apresentação desligada/ligada. Finalmente confrontar uma referência
externa com as mesmas condições e o mesmo ponto de amostragem no frame.

**Aceite sugerido:** a rota atual até resultados produz os mesmos registros em
execuções repetidas; qualquer divergência informa primeiro tick, entidade,
campo e valores. O teste de estoque continua aprovando pelas condições já
definidas no CMake.

## 5. Áudio: uma implementação concreta para estudar

O Unlocked separa três camadas:

1. A biblioteca AX recompilada constrói listas de comandos e blocos de
   parâmetros de voz; as pontes AI/DSP estão em
   [`hle_stubs.cpp`](../../melee-unlocked/port/runtime/hle/hle_stubs.cpp).
2. `ax_ucode.cpp` interpreta esses comandos e mistura áudio, acessando RAM/ARAM
   por uma interface de callbacks definida em
   [`ax_ucode.h`](../../melee-unlocked/port/runtime/hle/ax_ucode.h).
3. [`host/audio.cpp`](../../melee-unlocked/port/runtime/host/audio.cpp) entrega
   blocos de PCM estéreo de 32 kHz ao dispositivo por WASAPI, com fallback WinMM.

O mixer tem uma fronteira suficientemente pequena para estudo isolado: seu
teste compila e passa no Linux. Isso não elimina o trabalho de integração.
Nosso port precisa implementar a gestão das vozes, sincronizar os parâmetros,
avançar o DSP no tempo virtual e devolver o estado esperado pelo synth. Não
basta ligar uma saída SDL ao `AXAcquireVoice` atual.

Sequência proposta:

1. Exercitar uma voz sintética ADPCM e validar PCM, endereço corrente e término.
2. Implementar alocação/liberação e atualização dos parâmetros exigidos pelo
   `synth.c`, escolhendo uma representação explícita para os blocos AX.
3. Conectar ARAM e callbacks ao scheduler; conferir som real de menu/ataque.
4. Gravar WAV antes de depender da saída em tempo real.
5. Alimentar a saída SDL com buffer circular e medir underruns/overruns.

O backend de áudio do Unlocked lida com a diferença entre o relógio da simulação
e o relógio da placa de som ajustando gradualmente a taxa de consumo do buffer.
Essa técnica é transferível; WASAPI/WinMM não são o backend adequado para nosso
Linux. O tamanho de buffer deve ser medido aqui, não adotado como valor ideal.

[`jukebox.cpp`](../../melee-unlocked/port/runtime/hle/jukebox.cpp) contém leitura
de música HPS/DSP-ADPCM com looping, mas o acionamento responde aos comandos
Slippi que substituem a música original. É referência de formato, não uma
substituição automática do caminho de streaming vanilla.

[`wav_stats.py`](../../melee-unlocked/tools/wav_stats.py) ajuda a detectar
silêncio e amplitude. RMS não prova fidelidade: conferir também duração,
looping, canais, saturação e amostras esperadas.

## 6. GX: transferir a semântica e os testes

### 6.1 Texturas e paletas precisam pertencer ao draw

`TextureSnapshotCache` preserva bytes de imagem e paleta em snapshots imutáveis,
com deduplicação por conteúdo. O teste verifica alteração da paleta no mesmo
endereço, mudança em um mip posterior e validade após limpar a origem/cache.

Isso se conecta diretamente ao erro de TLUT registrado no nosso histórico:
capturar o estado depois do último draw pode usar uma paleta diferente daquela
que existia quando o objeto foi desenhado. A correção já consta do nosso
histórico; o ganho é ampliar a regressão com os casos do Unlocked.

Não substituir nossos leitores com validação de limites pelo decoder deles
sem manter as garantias atuais. O teste de snapshot é pequeno e não certifica
a segurança de todos os formatos ou streams malformados.

### 6.2 Fog, TEV indireto, swaps e profundidade

`gx_shader.cpp` oferece caminhos concretos de geração HLSL para fog, consultas
indiretas de textura, seleção de canais e cálculo de profundidade. Usar junto
de `gx_regs.h` para identificar as entradas da fórmula e expressá-las em nosso
[tev.cpp](../port/src/gx/tev.cpp), conforme a necessidade de cada cena.

As convenções de profundidade/projeção do D3D12 não devem ser copiadas
literalmente para OpenGL. Validar cada efeito com estado controlado e captura
equivalente. `a_bump` no TEV indireto e geração de coordenadas `GX_TG_BUMPn`
não são automaticamente a mesma implementação; nosso código ainda reporta
`bump texgen` em `tev_unmodelled_features`.

### 6.3 Cópias de EFB e fila de frames

EFB é o framebuffer do console; suas cópias podem alimentar texturas usadas
posteriormente. Em [`frame_queue.h`](../../melee-unlocked/port/runtime/gx/frame_queue.h)
e [`threaded_backend.cpp`](../../melee-unlocked/port/runtime/gx/threaded_backend.cpp),
frames são processados em ordem; quando há atraso, o renderer pode deixar de
apresentar frames intermediários, preservando a execução dos comandos.

Se paralelizarmos nosso renderer, descartar trabalho intermediário exige
preservar essas dependências. Testar sombra/refração e transições com consumidor
atrasado. O código atual limita a fila a 32 frames; o tracker menciona uma fila
de quatro em uma etapa anterior. São estados históricos diferentes, não uma
recomendação para nossa latência.

### 6.4 Cache de shaders e medição

O Unlocked normaliza chaves de shader pelos registradores relevantes e inclui
hashes dos fontes de shader/layout/backend na identidade do cache. Há receitas
de pré-compilação e pipelines genéricos enquanto o shader específico compila.
O tracker relata que simplesmente pular draws causou objetos piscando.

Nosso [`sdl_gl_renderer.cpp`](../port/src/render/sdl_gl_renderer.cpp) já guarda
programas por fonte GLSL. Antes de adicionar workers ou cache persistente,
medir quantas compilações novas e quanto tempo de compilação aparecem por luta.
Um fallback aproximado pode preservar geometria, mas deve ser identificado
como aproximação nas comparações de imagem.

[`benchmark_native.py`](../../melee-unlocked/tools/benchmark_native.py) fornece
um bom modelo: separar inicialização da partida, cache frio/quente, registrar
hash do executável/roteiro e percentis p95/p99. Seus intervalos de submissão CPU
não medem conclusão GPU nem latência física do controle até a tela.

## 7. CARD e DVD: contratos úteis, com adaptação de ABI

### Saves em GCI

`hle_card.cpp` implementa uma pasta de arquivos `.gci`: entrada de diretório de
64 bytes seguida dos blocos de dados de 8 KiB. O modelo implementado expõe
slot A e trata slot B como ausente. Há resultados de erro da API, completions
assíncronas e escrita por arquivo temporário seguida de renomeação.

Aplicação: dar persistência às regras/desbloqueios e permitir fixtures de save
para testes. Isso também ajuda a ampliar os cenários hoje limitados pela SSS
sem save. A ponte atual deles lê estruturas e callbacks em endereços PPC;
nosso backend precisará receber os tipos/ponteiros nativos corretos.

Aceite: criar, salvar, fechar e reabrir; conferir erro de arquivo inexistente,
capacidade e callback; executar cenários com pasta vazia e com fixture conhecida.

### I/O assíncrono com tempo previsível

`hle_dvd.cpp` lê em worker e agenda a conclusão para um prazo virtual fixo,
descrito como um quarto de frame, mantendo a ordem dos pedidos. Se o worker
ainda não terminou no prazo, a simulação espera. Portanto, o mecanismo reduz
bloqueios antecipados, mas não garante ausência de stalls.

Nosso [`os/dvd.cpp`](../port/src/os/dvd.cpp) já entrega operações pelo scheduler.
O ganho futuro é desacoplar a leitura física preservando instante e ordem
observados pelo jogo. Testar leituras consecutivas, falha/truncamento e callback
que agenda outra operação antes de otimizar throughput.

## 8. Determinismo, alta taxa de atualização e Slippi

### Ponto flutuante

Os helpers PPC incluem arredondamento para single, ajuste de mantissa, FMA,
conversões e estimativas de recíproco/raiz. O cabeçalho declara como alvo a
semântica Jit64 de Slippi Dolphin no caminho indicado; isso não é comprovação
de identidade universal com hardware.

Nosso status registra uso residual de `libm` e diferenças potenciais de
aliasing. Se a primeira divergência de gameplay apontar para matemática,
comparar operações específicas com vetores de teste, incluindo sinais de zero,
limites de conversão e arredondamento. Trocar funções isoladas pela referência
não certifica o determinismo do jogo inteiro.

### Apresentação acima de 60 Hz

O princípio aproveitável é manter a simulação em 60 Hz e trabalhar sobre
snapshots de apresentação. `authored_pose` captura canais de animação,
hierarquia de joints, pesos e matrizes de envelope; o solver amostra poses
fracionárias e preserva a pose atual quando a reconstrução não é suportada.
Parte do movimento usa extrapolação, sujeita a limites e descontinuidades.

O código também distingue interpolação entre dois estados, que introduz um
frame de atraso, de predição à frente do estado atual. A escolha exige medir
qualidade e latência. Respawn, mudança de ação, câmera, efeitos e reutilização
de endereços precisam de tratamento explícito.

Já temos FObj/AObj/JObj nativos: usar a separação entre simulação e apresentação
como referência, sem trocar a animação existente. Qualquer experimento deve
passar pelo teste de isolamento de estado da seção 4.

### Slippi exige mais que rede

`slippi_online.cpp` implementa savestates copiando regiões de RAM convidada e
excluindo faixas específicas de áudio/VI. `exi_slippi.cpp`, `slippi_net.cpp` e
`slippi_report.cpp` compõem outras partes do dispositivo e protocolo.

Esses endereços não representam os objetos do nosso heap nativo. Para rollback,
será preciso restaurar objetos, referências, RNG, filas e estado relevante sem
depender da localização ocasional das alocações. Isso deve orientar uma futura
estratégia de IDs/arenas/serialização, não bloquear o MVP offline.

O README anuncia online; o tracker contém evidência histórica mais restrita e
trechos que ainda pedem validação contra Dolphin. Nenhum replay/log de uma
sessão externa foi reproduzido aqui. Tratar compatibilidade como uma hipótese
a testar, e evitar tomar um par de instâncias iguais como prova de equivalência
com outro emulador.

## 9. Condições para reproduzir a referência

Há impedimentos concretos no checkout examinado:

- O [CMake principal](../../melee-unlocked/CMakeLists.txt) exige Windows x64/MSVC
  para habilitar o executável experimental.
- `native_animation` depende de `melee/src/sysdolphin/baselib/fobj.c`,
  `fobj.h` e `spline.c` por meio de
  [`generate_fobj_host.py`](../../melee-unlocked/tools/generate_fobj_host.py).
  A pasta **interna** `melee-unlocked/melee/` não está presente e é ignorada pelo
  Git. O nosso `../melee/` é outro checkout, não satisfaz automaticamente esse
  caminho. Para reproduzir, resolver essa dependência e fixar sua revisão.
- A geração de C++ em `port/generated/` é uma etapa separada a partir do DOL.
  A presença do recompiler não significa que esses fontes gerados existam.
- `HANDOFF_FABLE_3.md` e os relatórios citados no tracker não estão no checkout
  examinado; os números de desempenho e partidas ali descritos são relatos do
  projeto, não resultados reproduzidos nesta análise.

O [README](../../melee-unlocked/README.md) declara GPL-2.0-or-later e origem de
partes do runtime em Dolphin/Slippi. Os arquivos examinados trazem cabeçalhos
SPDX e há [LICENSE](../../melee-unlocked/LICENSE) no projeto. Registrar origem,
revisão e licença por componente se houver incorporação. Este levantamento não
avalia a compatibilidade de licenças nem transfere código entre os projetos.

## 10. Verificações executadas neste levantamento

Compilação isolada com `c++ -std=c++17 -O2`, fora dos checkouts, em diretório
temporário. Nenhum desses casos precisa de ISO ou acesso online.

| Verificação | Resultado observado | Alcance |
| --- | --- | --- |
| `ax_ucode_test.cpp` + `ax_ucode.cpp` | Passou | ADPCM sintético, escrita de posição no bloco de voz, interleaving e controle de mix |
| `texture_snapshot_test.cpp` + `gx_texture.cpp` | Passou | Paleta mutável, mips, ownership e deduplicação |
| `port/tests/dol_validation_test.py` | 2 testes passaram | Header truncado e limites de seções DOL |

Os resultados demonstram que esses recortes podem ser exercitados no Linux.
Não cobrem gameplay, backend D3D12, áudio real, performance, Slippi nem toda a
segurança dos parsers.

Comandos reproduzíveis a partir da raiz `project-melee/`:

```sh
review_dir=$(mktemp -d /tmp/melee-unlocked-checks.XXXXXX)

c++ -std=c++17 -O2 -I melee-unlocked/port/runtime/hle \
  melee-unlocked/port/tests/ax_ucode_test.cpp \
  melee-unlocked/port/runtime/hle/ax_ucode.cpp -o "$review_dir/ax-test"
"$review_dir/ax-test"

c++ -std=c++17 -O2 -I melee-unlocked/port/runtime/gx \
  melee-unlocked/port/tests/texture_snapshot_test.cpp \
  melee-unlocked/port/runtime/gx/gx_texture.cpp -o "$review_dir/texture-test"
"$review_dir/texture-test"

PYTHONDONTWRITEBYTECODE=1 python3 \
  melee-unlocked/port/tests/dol_validation_test.py
```

## 11. Backlog sugerido, sem implementação nesta tarefa

| Ordem | Entrega | Critério de conclusão |
| --- | --- | --- |
| 1 | Consolidar a rota existente de vitória por estoque | Teste de estoque aprovado, imagem dos marcos conferida e documentação coerente com o código |
| 2 | Trace canônico da luta | Duas execuções e modos de apresentação com o mesmo estado; diagnóstico do primeiro campo divergente |
| 3 | Primeira voz AX audível | Vetor sintético correto, WAV de som real e callbacks/estado de voz coerentes |
| 4 | Correções GX guiadas por capturas | Um efeito por vez, testes de paleta/mips e cenas de sombra/refração sem regressão |
| 5 | Mais conteúdo e saves | Próximo personagem escolhido por dependências de dados; fixture GCI reproduzível e luta completa |
| 6 | Desempenho e apresentação desacoplada | Medição frio/quente, p95/p99 e isolamento da simulação preservado |

**Decisão sugerida:** manter a arquitetura atual e aproveitar componentes
pequenos com testes junto. O Unlocked é especialmente valioso para transformar
uma dúvida de implementação em uma comparação observável: qual estado, qual
amostra de áudio ou qual draw difere, e em que momento.
