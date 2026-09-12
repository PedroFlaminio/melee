# Fluxo de luta nativo

## Meta vertical

Chegar a uma partida local com dois jogadores, a partir de `StartMeleeData`,
sem emulação do GameCube:

1. menu produz `StartMeleeData`;
2. scene manager seleciona uma cena VS e o estágio;
3. `Player_InitAllPlayers` materializa os jogadores;
4. `fighter.c` e `ground.c` executam o loop por frame;
5. PAD, DVD/HSD, renderer e áudio host atendem o fluxo.

## Primeiro ponto de entrada

`src/melee/pl/player.c:Player_InitAllPlayers` é o primeiro entrypoint de
runtime dos jogadores. As cenas de demonstração em `src/melee/vi/` mostram a
sequência mínima: inicialização de objetos de lutador, `Player_InitAllPlayers`
e entrada de cena. O fluxo VS deve reutilizar essa sequência, mas receber
`StartMeleeData` produzido por menu.

## Gates de compilação

- Compilado: controlador de menu (`mnmain.c`), scene manager (`gmscene.c`) e
  rota VS (`gmvsmode.c`, `gmvsmelee.c`, `gmvs.c`). Ainda falta conectá-los às
  facades do host e executar a transição.
- Executado: a entrada de menu percorre o HSD Pad e o avaliador original; a
  saída usa um mapeamento host compativel com `mn_80229624`, sem carregar o
  grafo visual ainda pendente. A configuração VS usa o armazenamento original
  de `gmMainLib` por uma ABI host sem layouts PPC expostos.
- Compilado: núcleo de partida (`gmmain.c`/`gmmain_lib.c`), `player.c`,
  `fighter.c` e `ground.c` sem assembly PPC. O próximo recorte deve resolver
  suas facades e ligá-los a um entrypoint host, não apenas ao arquivo estático.
- Executado: o escalonador de frame original. `HSD_GObjInit` sobe com os tetos
  de prioridade que `gmScene_Init` usa (`gproc_pri_max` 0x18) e
  `HSD_GObj_RunProcs` roda por frame no host. É esse laço que `gmscene.c`,
  `player.c`, `ground.c` e `fighter.c` esperam para executar por frame.
- Executado: a camada de objetos gráficos HSD. `ground.c` e `fighter.c` criam
  GObjs e anexam JObj/CObj/LObj, e esses caminhos agora chegam ao código real
  em vez dos substitutos que abortavam. Cenas e modelos do disco entram por
  `HSD_JObjLoadJoint` sobre descritores materializados em layout host; ver
  `docs/native_port_status.md`.
- Executado: o caminho de render original. `HSD_JObjDispAll` percorre a árvore
  carregada e as display lists do asset chegam ao recorder GX do host, que é o
  mesmo caminho que `ftdrawcommon.c` e `itdraw.c` tomam por frame.
- Próximo bloqueio: apresentar o que já desenha. O recorder registra a
  geometria e não produz pixels, então nenhuma imagem sai da camada gráfica
  original ainda.
- A matemática paired-single, o subset de estado GX e a camada VI que essa
  camada consome já estão prontos e testados. O laço de frame já tem as duas
  metades que precisava: `HSD_GObj_RunProcs` para a simulação e o retrace de
  VI para a apresentação, ambos determinísticos e sem dormir.
- O heap do OS já roda: o alocador original entrega blocos em endereços de 64
  bits reais, e `initialize.c` entrou junto com a camada de objetos gráficos.
  Detalhes em `docs/native_port_status.md`.
- Implementar as facades GX, áudio e DVD que surgirem durante esse recorte,
  apenas conforme necessárias.

## Critério de aceite

Dois controles host selecionam lutadores, entram em uma cena de estágio, os
lutadores recebem input em ticks determinísticos e a cena é apresentada pelo
backend SDL/OpenGL.
