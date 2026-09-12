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
- [x] Primeiro subconjunto de materiais GX concluido para os formatos presentes
  em `GmPause.dat` (I4/IA4), do HSD ate o backend SDL/OpenGL.
- [x] Testes sinteticos de disco e HSD.
- [x] Primeiros modulos originais compilados nativamente: RNG, tempo, vetores,
  controlador, memoria, objalloc/list e fila `devcom` da baselib.
- [x] Presets de debug/sanitizers e workflow multiplataforma.

## Em andamento

- [ ] Compilar todo o codigo relevante sem assembly PPC.
- [ ] Resource manager runtime consumindo o manifesto extraido.
- [ ] Cancelamento, streaming e prioridade completa da API DVD.
- [ ] Backend SDL/SDL_GameController para alimentar a entrada do host.
- [ ] Loader HSD com schemas Disk/Runtime e referencias ciclicas.

## Proximos gates

1. Inicializar o primeiro grafo de audio sem DSP/ARAM fisico.
2. Compilar o primeiro fluxo de menu sobre as facades host.
3. Implementar TLUT e texturas paletizadas (`C4`, `C8`, `C14X2`) como extensao
   do renderer de materiais.

## Limitacoes atuais

- Os assets `GALE01` extraidos estao disponiveis apenas em `assets-local`, que
  permanece ignorado pelo Git e nao faz parte de builds ou artefatos publicos.
- O executavel ainda nao chama `gmMain`.
- GX (alem do recorder/VCD/VAT), AX, CARD, streaming DVD e THP ainda nao estao
  implementados. PAD e DVD assincrono tem pontes basicas; os backends completos
  ainda faltam.
- A build matching nao foi executada porque `orig/GALE01/sys/main.dol` nao esta
  presente; as mudancas compartilhadas estao isoladas por `MELEE_HOST`.
