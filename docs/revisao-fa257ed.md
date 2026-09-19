# Revisão técnica a partir de `fa257ed`

Data: 19/09/2026. Repositório: `melee`.

**Parecer:** há avanços úteis, mas o conjunto ainda precisa de correções antes de ser considerado estável. Os principais bloqueadores são corrupção de memória nos tradutores, tradução incorreta de parâmetros de cenários e profundidade indefinida no shader. A suíte existente passa, mas não cobre suficientemente as funcionalidades adicionadas.

A revisão adota o rigor de uma revisão sênior de uma entrega júnior: questiona pressupostos, contratos, tratamento de falhas e evidências de funcionamento. As observações são sobre o código, sem inferir a experiência de quem o escreveu.

## Escopo e método

- Intervalo **inclusivo**: `fa257ed6da1f80060a1496a65f9de6bda3102a61` até `1348ec4a59dced31313bcdfb2c25c14e12c10ec9`.
- Base de comparação: `e82e7c554138ea540e07f20ebf2dbd3d4ec65446`, pai de `fa257ed`.
- 21 commits, 70 arquivos, 22.877 linhas adicionadas e 2.138 removidas. Os dois headers de terceiros respondem por 15.512 linhas adicionadas.
- Foram examinados histórico, diff acumulado, implementação final, consumidores das estruturas alteradas, testes, scripts e documentação. Defeitos corrigidos dentro do próprio intervalo não são apresentados como pendentes.
- O foco foi o código próprio e a integração das dependências. Não foi realizada uma auditoria integral dos algoritmos internos de `stb_image.h` e `xxhash.h`, nem engenharia reversa de `port/test_ui`.
- Foram executados builds, testes existentes, inspeção estrutural dos assets locais e reproduções isoladas. Não foram realizados testes interativos de GPU, controle físico, áudio ou Windows. Os achados desses caminhos são identificados como análise estática.
- Nenhum código de produção foi corrigido nesta revisão. Este documento é a entrega; arquivos auxiliares de diagnóstico ficaram em `/tmp`.

## Prioridades

**P1:** corrigir antes de ampliar testes de uso ou distribuir uma versão. **P2:** defeito funcional ou de robustez que deve entrar na próxima rodada de correções. Melhorias de manutenção estão em seção separada.

| ID | Prioridade | Problema | Evidência principal |
| --- | --- | --- | --- |
| R01 | P1 | Objeto de Samus subalocado em 64 bits | Código e reprodução isolada com ASan |
| R02 | P1 | Cópia de índices escreve além da alocação | Código, asset de Kirby e reprodução isolada com ASan |
| R03 | P1 | Offsets de cenários tratados como ponteiros nativos | Contrato do reader e asset de Battlefield |
| R04 | P1 | Parâmetros desconhecidos são silenciosamente zerados | Inspeção de 11 arquivos afetados |
| R05 | P1 | Cenários diferentes recebem o mesmo layout incorreto | Ramos inalcançáveis e tamanhos dos assets |
| R06 | P1 | Shader deixa a profundidade indefinida | Código e especificação GLSL |
| R07 | P2 | Menu descarta eventos de encerramento e conexão de controle | Análise estática |
| R08 | P2 | Alternar texturas customizadas não atualiza o cache | Análise dos dois caches |
| R09 | P2 | Anisotropia não vale para texturas novas nem após reiniciar | Análise estática |
| R10 | P2 | MSAA valida o framebuffer errado e ignora falhas | Análise estática |
| R11 | P2 | Capturas BMP não resolvem MSAA | Análise estática do caminho de leitura |
| R12 | P2 | Falha de abertura pode destruir um ImGui não inicializado | Análise dos caminhos de erro |
| R13 | P2 | `npm run play` não inicia o jogo | Comando executado |
| R14 | P2 | `Unlock Everything` tem comportamento incompatível com um checkbox | Análise do boot, da UI e da persistência |
| R15 | P2 | Teste aleatório aceita sucesso sem comprovar que houve luta | Reprodução com saída simulada |

## Correções propostas

### R01 — Alocar o tamanho nativo do objeto de Samus

**Local:** [game_data_translators.c](../port/src/game/game_data_translators.c), linhas 3852–3898, especialmente 3865.

`samus_throw_beam_model` declara uma estrutura com quatro `void*`, mas aloca `0x10` bytes. Em x86-64 ela ocupa 32 bytes. As atribuições a `beam->x8` e `beam->xC` escrevem nos deslocamentos 16 e 24, além dos 16 bytes reservados. Como o vetor de animações é alocado em seguida na mesma arena, esses campos também podem se sobrepor aos dados desse vetor.

**Verificação:** a função original, extraída para um programa com reader simulado e alocações individuais, produziu `AddressSanitizer: heap-buffer-overflow`, com escrita de 8 bytes. O carregamento normal de `PlSs.dat` não acusou ASan; isso não elimina a sobreposição dentro da arena.

**Correção:** usar um tipo nomeado, `sizeof(*beam)` e seu alinhamento nativo. Revisar também o número fixo de quatro entradas do vetor contra o formato real do asset.

**Regressão:** carregar os dados de Samus e verificar individualmente modelo, quatro animações e matanim; executar grab/grapple sob sanitizadores com detecção dos limites de cada objeto da arena.

### R02 — Corrigir o deslocamento duplicado na cópia dos índices

**Local:** [game_data_translators.c](../port/src/game/game_data_translators.c), linhas 1009–1017 e 2832–2873; chamada problemática na linha 2858.

`copy_bytes` adiciona `begin` tanto à origem quanto ao destino. A chamada `copy_bytes(reader, target, yoshi2->xC, 12, extent)` já passa um destino deslocado em 12 bytes e adiciona mais 12. Para `extent == 16`, escreve nos bytes 24–27 de uma alocação de 16 bytes, deixando o trecho esperado sem copiar.

**Verificação:** o terceiro subbloco de `x1C` em `PlKb.dat` tem extent 16. A reprodução isolada da função e do helper originais produziu `heap-buffer-overflow`, escrita de 1 byte. A função também atende Yoshi; os subblocos inspecionados de `PlYs.dat` tinham 4, 8 ou 12 bytes, portanto não acionaram esse ramo. Não se deve afirmar que ambos os personagens reproduziram o mesmo overflow com esses assets.

**Correção:** passar a base `yoshi2` ao helper para manter os offsets absolutos. Validar tamanhos mínimos antes das escritas e confirmar o formato de cada variante, em vez de pressupor que todas são três inteiros seguidos por bytes.

**Regressão:** testar extents 4, 8, 12 e 16, valores reconhecíveis na cauda e guardas após a alocação; incluir o carregamento de Kirby.

### R03 — Materializar os alvos dos ponteiros de cenários

**Local:** [yakumono_param.c.inc](../port/src/game/yakumono_param.c.inc), linhas 27–40 e demais atribuições `(void*)(uintptr_t)target`.

`melee_host_hsd_reader_pointer` retorna um **offset da seção de dados**, conforme [hsd_host_archive.cpp](../port/src/assets/hsd_host_archive.cpp), linhas 605–618. Converter esse inteiro para `void*` não produz um endereço válido do processo.

Exemplo concreto: `GrNBa.dat` tem um bloco de 8 bytes cujos alvos são `0x34088` e `0x340a4`. O tradutor entrega esses valores como ponteiros. Em [grbattle.c](../src/melee/gr/grbattle.c), linhas 401–407, os campos são encaminhados a `grMaterial_801C9604`, que os usa como scripts. A tradução informa sucesso mesmo sem construir esses scripts.

**Correção:** materializar cada alvo segundo seu tipo: lista de inteiros, script com conversão de comandos, joint ou animação. Usar payload bruto somente quando o consumidor realmente espera bytes sem tradução. Enquanto um tipo não estiver suportado, reportar falha explícita.

**Regressão:** verificar os valores e a propriedade dos objetos apontados e executar a troca de overlays de Battlefield; não basta testar que o símbolo retornou um ponteiro diferente de `NULL`.

### R04 — Não substituir parâmetros desconhecidos por zeros

**Local:** [yakumono_param.c.inc](../port/src/game/yakumono_param.c.inc), linhas 880–883; [hsd_host_archive_test.cpp](../port/tests/hsd_host_archive_test.cpp), teste de `yakumono_param` próximo da linha 1400.

O `default` aloca um bloco zerado e retorna sucesso para dados que não reconhece. A verificação anterior, que recusava parâmetros não suportados, foi removida junto com seu teste negativo, sem uma validação equivalente. Isso faz `melee_host_stage_symbols_check` deixar de detectar conteúdo não traduzido.

**Verificação:** uma leitura dos headers, relocations, símbolos públicos e limites dos blocos, seguindo o critério de `translator_extent`, encontrou 43 blocos `yakumono_param` não nulos. Onze caem no `default`:

| Asset | Extent em bytes |
| --- | ---: |
| `GrCs.dat` | 328 |
| `GrGb.dat` | 164 |
| `GrIm.dat` | 316 |
| `GrNFg.dat` | 24 |
| `GrNKr.dat` | 388 |
| `GrNPo.dat` | 532 |
| `GrNSr.dat` | 296 |
| `GrOt.dat` | 104 |
| `GrOy.dat` | 28 |
| `GrTPr.dat` | 4 |
| `GrZe.dat` | 400 |

**Impacto:** timers, velocidades, limites e ponteiros podem virar zero silenciosamente, alterando a lógica ou causando crashes posteriores. Esses números demonstram seleção incorreta do tradutor, não que cada cenário foi executado até falhar.

**Correção:** recusar explicitamente layouts desconhecidos e implementar tradutores identificados por cenário. Restaurar o teste negativo e acrescentar fixtures para os tamanhos reais. O ramo de Ice Mountain também precisa corrigir offsets: lê `x9C` em `0xA0` e procura pointers em `0xAE/0xB2/0xB6`, enquanto o asset registra `0xAC/0xB0/0xB4`. Ajustar apenas o `case 212` para 316 não resolve o layout.

**Regressão:** testar conteúdo dos parâmetros, erros para tipos não suportados e execução dos hazards de cada cenário.

### R05 — Não inferir o tipo de cenário apenas pelo tamanho

**Local:** [yakumono_param.c.inc](../port/src/game/yakumono_param.c.inc), linhas 192–225, 287–326 e 377–452.

Os `if (1)` tornam várias alternativas inalcançáveis. O bloco de 52 bytes sempre usa `grOldpupupu`, nunca `grKraid`; o de 76 sempre usa `grFourside`, nunca `grInishie2`; o de 84 sempre usa `grIzumi`, nunca `grInishie1` ou `grPStadium`.

**Verificação:** os assets locais `GrKr.dat`, `GrI2.dat`, `GrI1.dat` e `GrPs.dat` têm respectivamente 52, 76, 84 e 84 bytes. Os layouts incluem combinações diferentes de `u16`, `u32` e floats; aplicar outro layout pode trocar a ordem dos pares de 16 bits e corromper os valores sem gerar erro de memória.

**Correção:** selecionar por identidade do arquivo/cenário ou por schema explícito. Tamanho e relocations devem validar a escolha, não substituí-la.

**Regressão:** fixtures de dois cenários com o mesmo tamanho, conferindo campos de 16 e 32 bits e ao menos um comportamento do hazard.

### R06 — Definir a profundidade em todos os caminhos do shader

**Local:** [tev.cpp](../port/src/gx/tev.cpp), linhas 579 e 764–766; [sdl_gl_renderer.cpp](../port/src/render/sdl_gl_renderer.cpp), linhas 382–383 e 403–436.

O fragment shader escreve `gl_FragDepth` somente se `u_z_texture_op == 1`. Quando um shader contém essa escrita, os caminhos que não a executam podem produzir profundidade indefinida. Isso afeta também desenhos comuns, com Z texture desativada. A exigência consta da [especificação GLSL 3.30, seção 7.2](https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.3.30.pdf).

Há ainda três lacunas na implementação: a localização do uniform é obtida, mas seu valor não é enviado em `set_program_uniforms`; `GX_ZT_REPLACE` vale 2, enquanto 1 é `GX_ZT_ADD`; formato e bias não são considerados. Portanto, o suporte anunciado a depth textures não está conectado corretamente ao estado GX.

**Correção:** preservar `gl_FragCoord.z` no caminho normal, ou gerar uma variante sem escrita de profundidade. Para Z texture, enviar o estado e implementar corretamente ADD/REPLACE, formato e bias, aproveitando a referência já existente no rasterizador de CPU.

**Regressão:** comparar profundidade, oclusão e cor CPU/GPU com Z texture desativada, ADD e REPLACE. O teste atual de cor TEV, sozinho, não prova o comportamento do depth buffer. Nenhum artefato visual específico foi reproduzido nesta revisão.

### R07 — Separar captura de input de eventos de ciclo de vida

**Local:** [settings_ui.cpp](../port/src/render/settings_ui.cpp), linhas 74–78; [sdl_gl_renderer.cpp](../port/src/render/sdl_gl_renderer.cpp), linhas 2225–2243.

`process_settings_event` retorna `WantCaptureKeyboard || WantCaptureMouse` independentemente do tipo do evento. O chamador faz `continue` antes de tratar `SDL_EVENT_QUIT`, conexão e remoção de gamepad. Com o menu capturando input, fechar a janela pode ser ignorado. O Esc anunciado para alternar o menu também pode ser consumido antes de chegar ao toggle.

**Correção:** processar encerramento e conexão de dispositivos sempre; aplicar a captura apenas aos eventos de teclado/mouse correspondentes. Definir explicitamente a prioridade do Esc.

**Regressão:** abrir o menu, fechá-lo por Esc, encerrar pela janela e conectar/desconectar controle enquanto a UI tem foco.

### R08 — Invalidar texturas decodificadas e enviadas à GPU ao alternar o recurso

**Local:** [sdl_gl_renderer.cpp](../port/src/render/sdl_gl_renderer.cpp), linha 2128; [main.cpp](../port/src/main.cpp), linhas 839–912.

O checkbox altera apenas `g_custom_textures_enabled`. `TitleTextureCache` decodifica novamente somente quando a chave muda ou uma cópia de EFB altera a geração. A opção não participa desses critérios. Texturas originais já carregadas continuam originais ao habilitar; substituições já carregadas continuam ativas ao desabilitar. O cache GL também depende da geração da imagem.

**Correção:** introduzir uma revisão das configurações de textura e propagá-la aos dois caches, invalidando/redecodificando as imagens afetadas. Preservar a validade dos ponteiros que o presenter usa como chave.

**Regressão:** carregar uma cena com substituição conhecida e alternar o checkbox nos dois sentidos, sem reiniciar nem trocar de cena.

### R09 — Aplicar anisotropia na criação de cada textura

**Local:** [sdl_gl_renderer.cpp](../port/src/render/sdl_gl_renderer.cpp), linhas 586–613, 1453–1458 e 2142–2145.

A anisotropia só é aplicada às texturas existentes quando algum controle da UI muda. `create_texture` não usa `g_anisotropy_level`. Assim, a configuração carregada do disco não é aplicada às texturas novas; texturas de outra cena e reuploads por geração também perdem o valor selecionado.

**Correção:** centralizar a aplicação dos parâmetros do sampler e chamá-la na criação e atualização. Verificar suporte à extensão e limitar o valor à capacidade reportada pela GPU, em vez de presumir 16x.

**Regressão:** selecionar 8x, mudar de cena, recriar uma textura e reiniciar o programa; conferir o parâmetro GL em todos os casos.

### R10 — Validar ambos os framebuffers e tratar falhas de MSAA

**Local:** [sdl_gl_renderer.cpp](../port/src/render/sdl_gl_renderer.cpp), linhas 736–785, 1384–1399 e 2147.

Com MSAA, o construtor termina com `resolve_framebuffer` ligado. `complete()` verifica o framebuffer atualmente ligado, portanto testa o resolve e pode aceitar um framebuffer multisample incompleto. Além disso, a seleção de 8x não consulta os limites da GPU e o retorno de `configure_render_target` é ignorado pela UI.

**Impacto:** uma configuração não suportada ou uma falha de alocação pode ser salva como aplicada e resultar em renderização vazia ou inconsistência entre menu e estado efetivo.

**Correção:** validar explicitamente cada FBO, consultar capacidades e aplicar a mudança como transação: criar, validar, substituir o anterior e só então persistir. Em erro, manter a configuração anterior e informar o motivo.

**Regressão:** simular limite inferior a 8 amostras e falha de criação; verificar fallback, manutenção do alvo anterior e ausência de persistência de um valor rejeitado.

### R11 — Resolver MSAA antes de salvar BMP

**Local:** [sdl_gl_renderer.cpp](../port/src/render/sdl_gl_renderer.cpp), linhas 789–800 e 2204–2214.

`save_bmp` liga diretamente o framebuffer multisample e chama `glReadPixels`. O resolve existe apenas dentro do caminho de apresentação visível. Com MSAA salvo nas preferências, o presenter oculto também cria esse alvo, mas a captura não resolve as amostras antes da leitura. O erro GL não é consultado, de modo que o arquivo pode ser gravado sem pixels válidos.

**Correção:** compartilhar uma rotina de resolve entre apresentação e captura; ler o FBO de uma amostra e propagar falhas de leitura. Preferencialmente permitir configurações explícitas para testes de screenshot, independentes das preferências pessoais.

**Regressão:** salvar a mesma cena oculta com AA Off, 2x, 4x e 8x suportados; validar dimensões, pixels e erros GL.

### R12 — Destruir a UI somente depois de sua inicialização

**Local:** [sdl_gl_renderer.cpp](../port/src/render/sdl_gl_renderer.cpp), linhas 1874 e 1912–1922; [settings_ui.cpp](../port/src/render/settings_ui.cpp), linhas 64–69.

Quando `configure_render_target` falha, `open` transfere o estado para `state_` e retorna antes de chamar `init_settings_ui`. O destrutor chama `destroy_settings_ui` incondicionalmente. O backend OpenGL do ImGui exige estado previamente inicializado e contém uma asserção para shutdown sem backend.

**Correção:** registrar quais recursos foram inicializados ou encapsular cada um em um objeto responsável pelo próprio ciclo de vida. Verificar também os retornos da inicialização dos backends.

**Regressão:** injetar falha na criação do render target e nos backends; `open` deve retornar erro e liberar os recursos sem asserção ou crash.

### R13 — Separar compilação e execução no script npm

**Local:** [package.json](../package.json), linha 6.

Falta um separador entre `cmake --build ... -j2` e o caminho do executável. Ao executar `npm run play`, o CMake recebeu o executável como argumento adicional e imprimiu `Unknown argument ./build/host-release/port/melee-pc`; o jogo não iniciou.

**Correção:** usar, por exemplo, `cmake --build --preset host-release --target melee-pc -j2 && ./build/host-release/port/melee-pc --play assets-local`. Documentar a configuração inicial com `cmake --preset host-release` ou incluí-la no fluxo. Se Windows for suportado por esse comando, tratar caminho e extensão do binário.

**Regressão:** executar o comando em uma árvore configurada e em uma árvore nova; verificar que falhas de build impedem o lançamento e que um build bem-sucedido o inicia.

### R14 — Definir um contrato coerente para `Unlock Everything`

**Local:** [gmmain_lib.c](../src/melee/gm/gmmain_lib.c), linhas 1315–1316; [settings_ui.cpp](../port/src/render/settings_ui.cpp), linha 182; [sdl_gl_renderer.cpp](../port/src/render/sdl_gl_renderer.cpp), linhas 1420–1485 e 2124–2126.

O boot já chama `gm_80164F18` incondicionalmente, mesmo com o checkbox desmarcado. Marcar o controle passa a chamar o desbloqueio completo em cada apresentação, mas desmarcar não desfaz as alterações. `unlock_all` também não é salvo nem carregado com as demais opções.

**Correção:** decidir se o produto mantém tudo desbloqueado por padrão ou oferece uma ação explícita. No segundo caso, um botão de execução única descreve melhor a operação. Se a intenção for uma preferência persistente, implementar sua persistência e semântica sem prometer reversão inexistente. Evitar mutações de progresso dentro do loop de composição de frames.

**Regressão:** verificar boot, clique, desmarcação e reinício, incluindo o estado de personagens, cenários e progresso salvo. A decisão de produto deve preservar qualquer preferência prévia do usuário por personagens desbloqueados.

### R15 — Não classificar uma rota interrompida como partida validada

**Local:** [random_cpu_matches.py](../port/tools/random_cpu_matches.py), linhas 453–465.

No modo `--until-end`, basta a saída conter `stopped at frame` para a execução receber `status = "ok"`. Essa condição é verificada antes da conferência dos personagens e não exige entrada na cena de luta nem resultado. O próprio STOP é emitido ao atingir o orçamento, inclusive se a partida não terminou.

**Verificação:** chamando `run_match` com retorno simulado de processo `0` e somente `stopped at frame 2000`, o resultado foi `status=ok`, sem `match_from`, `selected` ou `result`.

**Correção:** separar `partida concluída`, `orçamento atingido`, `seleção incorreta` e `luta não iniciada`. Validar entrada na cena, personagens e estágio independentemente do modo de término. Um smoke test que sobreviveu ao orçamento pode ser útil, mas deve ser identificado como tal.

**Regressão:** testar saídas de menu sem luta, seleção errada, luta em andamento no STOP e partida efetivamente concluída.

## Implementação realizada

As correções abaixo foram aplicadas após esta revisão.

| Achado | Situação | Implementação |
|---|---|---|
| R01 | Corrigido | A alocação de `samus_throw_beam_model` usa `sizeof` do tipo nativo. |
| R02 | Corrigido | A cópia de dados variáveis de Yoshi/Kirby passa a base da alocação ao helper. |
| R03–R05 | Corrigido com recusa segura | Apenas o bloco conhecido de Battlefield materializa seus dois fluxos de comandos; blocos não-zero sem esquema específico agora falham, em vez de converter offsets em ponteiros, escolher um ramo por `if (1)` ou zerar dados. |
| R06 | Corrigido parcialmente | O shader sempre inicializa `gl_FragDepth`, recebe operação e bias; `GX_ZT_REPLACE` passa a ser a operação que altera a profundidade. Ainda falta transportar a textura de profundidade em 24 bits para equivalência total com o rasterizador de referência. |
| R07 | Corrigido | Quit, Escape e eventos de gamepad são processados antes da captura do ImGui; a captura depende da categoria do evento. |
| R08 | Corrigido | A opção de texturas personalizadas tem revisão própria, que força a redecodificação e o reenvio à GPU. |
| R09 | Corrigido | Anisotropia é aplicada à criação e à atualização das texturas, limitada ao máximo exposto pelo driver. |
| R10–R11 | Corrigido | Os dois FBOs são verificados e o caminho BMP resolve a cor MSAA antes de `glReadPixels`; níveis MSAA acima do máximo do driver são recusados. |
| R12 | Corrigido | A destruição do ImGui só acontece após sua inicialização. |
| R13 | Corrigido | O script npm separa build e execução com `&&`. |
| R14 | Corrigido | Desbloqueio virou botão de ação única, não é persistido e não é aplicado em cada frame. |
| R15 | Corrigido | `--until-end` exige entrada em luta, seleção esperada e resultado antes de marcar sucesso. |

O suporte aos outros `yakumono_param` não foi fingido como completo: eles passam a falhar com mensagem explícita até que cada esquema seja associado à identidade do estágio e seus alvos sejam materializados. Isso reduz cobertura de estágios com parâmetros não-zero, mas elimina a corrupção de memória e de estado que a inferência por tamanho causava.

## Melhorias de manutenção e cobertura

1. **Fixar a versão do ImGui.** [imgui.cmake](../port/cmake/imgui.cmake), linha 8, usa `GIT_TAG master`. Builds do mesmo commit podem obter APIs diferentes. Usar um commit fixo, registrar a versão validada e oferecer um fluxo offline com dependências já disponíveis. O build executado usou a dependência local existente, não comprovou uma instalação limpa.

2. **Fortalecer o alocador no modo de diagnóstico.** A arena em [hsd_materialize.cpp](../port/src/assets/hsd_materialize.cpp), linhas 531–546, não delimita cada subalocação para ASan. Adicionar redzones/poisoning ou um modo com alocações separadas permite detectar R01/R02 na integração. Acrescentar validação de tamanho, falha do reader e retorno da alocação aos tradutores gerados.

3. **Substituir geração textual por schemas verificáveis.** `yakumono_param.h` duplica tipos do jogo e o `.c.inc` contém seleção ambígua e offsets inconsistentes. `gen_translators_script.py` usa caminho absoluto da máquina e modelos que já não correspondem à assinatura atual de `fighter_data`. Definir campos de disco, tamanhos, alinhamento e regras de pointer translation numa fonte única; gerar código reprodutível e conferir o resultado em CI. `sizeof` do host não deve determinar automaticamente quantos bytes existem no disco.

4. **Retirar artefatos experimentais do fluxo principal.** Os cinco `port/patch_*.py` reescrevem arquivos por texto e não constituem migrações seguras ou idempotentes. `gen_translators.py` e `test_present.cpp` são placeholders; `test_ui.cpp` é um programa manual sem integração no CTest; `port/test_ui` é um binário versionado. Remover ou mover experimentos para uma área claramente identificada e manter exemplos compiláveis por targets próprios. `play_cpu_match.py` apenas abre o jogo interativamente: renomear ou implementar a partida CPU prometida.

5. **Remover a UI antiga e aplicar somente a mudança necessária.** `draw_video_menu`, `draw_fps_counter`, glyphs, display list e estado do menu anterior permanecem junto do ImGui. `cycle()` continua sendo testado, mas não é o caminho usado pelos controles novos. Extrair parsing/persistência e aplicação das opções para código testável. Mudar Show FPS ou anisotropia não deveria recriar o FBO, reaplicar modo da janela e alterar VSync.

6. **Robustecer descoberta e decodificação de texturas.** `custom_textures.cpp` constrói o índice uma única vez, usa o diretório de trabalho e resolve nomes duplicados pela ordem de iteração. Indexa `.dds`, mas só procura `.png` e usa um decoder sem suporte DDS. Adicionar prioridade determinística, diagnóstico de arquivo recusado, refresh explícito e validação de dimensões antes de converter `int` para `uint16_t`. Tratar erros durante a iteração recursiva, não apenas no construtor. Validar a compatibilidade de nomes/hashes com fixtures reais do pack pretendido antes de anunciá-la.

7. **Rever o contrato de partida local.** [local_match.c](../port/src/game/local_match.c), linhas 74–76, mudou `melee_host_prepare_local_two_player_match` para criar P2 como CPU nível 9. O teste foi alterado para aceitar isso, mas nome e contrato continuam descrevendo a preparação anterior. Tornar tipo e nível parâmetros, ou criar uma função específica para humano versus CPU. O uso identificado atualmente é o diagnóstico; não há evidência de que essa função altere todas as partidas interativas de dois jogadores.

8. **Persistência versionada e observável.** O leitor agora aceita campos opcionais, o que é uma melhoria, mas o formato continua posicional e a escrita trunca diretamente o arquivo, sem informar falha. Adotar versão/schema, escrita temporária seguida de substituição e retorno de erro. Testar arquivos antigos, truncados e valores inválidos; evitar duplicar enum → valor efetivo no loader e no renderer.

9. **Medir simulação e apresentação separadamente.** As taxas altas reapresentam o mesmo frame; não existe interpolação implementada. Validar que 60 Hz de simulação se mantém em monitores de 60/120/144/165 Hz, em Unlimited e após pausas/resize. Medir também latência de input e áudio. Normais/tangentes adicionadas ao stream ainda não constituem bump mapping; medir o custo desse stream antes de mantê-lo sem consumidor.

10. **Atualizar a documentação com evidências atuais.** `port-characters-stages.md` declara “Todos rodam!” e “livre de crashes de dados”, afirmações incompatíveis com R01–R05. O plano de vídeo diz que a projeção não muda, mas o renderer já modifica projeções perspective e ortho. O README adia instruções de build que existem em outro documento. Publicar uma matriz de funcionalidades com cenário/personagem, ação testada, build, plataforma e commit; diferenciar tradução, carregamento e funcionamento em partida. Preservar o registro histórico de `fa257ed` como resultado daquele commit, sem apresentá-lo como validação do HEAD atual.

11. **Expandir os testes onde o código mudou.** Cobrir todos os novos tradutores com valores esperados, regressões de layout/endianness, ciclo de vida do ImGui, persistência, caches e MSAA. Comparar Debug e Release em rotas determinísticas. Manter testes negativos quando um formato continua não suportado. Registrar separadamente “processo terminou com sucesso”, “sem erros ASan” e “sem diagnósticos UBSan”; o histórico de `fa257ed` já reconhece 37 pontos de UBSan.

## Validações executadas

| Verificação | Resultado | Limite da evidência |
| --- | --- | --- |
| `cmake --build --preset host-debug -j2` | Sucesso | Build incremental; houve warnings no core |
| `ctest --preset host-debug --output-on-failure -j2` | **26/26**, 106,52 s | Inclui **227/227** casos no executável unitário; não testa a UI interativa |
| `cmake --build --preset host-sanitize -j2` | Sucesso | Não executa por si só caminhos problemáticos |
| CTest sanitize: extract, unitários, command-layout, dados de Mario e Link | **5/5 após ajuste de ambiente** | Os dois testes de assets inicialmente falharam por ptrace/LeakSanitizer; passaram fora do sandbox |
| `--load-archive` sanitize para Samus, Yoshi, Kirby, Ice Mountain e Battlefield | Símbolos reportados como traduzidos | Não prova layout correto, valores nem execução do gameplay |
| Funções de R01/R02 com reader simulado e alocações individuais | **Dois heap-buffer-overflow com ASan** | Reprodução isolada das funções, não crash reproduzido durante partida |
| Inspeção de blocos `yakumono_param` dos assets locais | **11/43 blocos não nulos caem no fallback zerado** | Não executa os 43 cenários |
| `npm run play` | Argumento inválido no CMake; jogo não lançado | Executado no ambiente Linux atual |
| `run_match` com saída simulada contendo apenas STOP | Retornou `ok` indevidamente | Teste do classificador, sem iniciar uma partida |

O LeakSanitizer não funciona no ambiente rastreado de algumas execuções. Para os probes de carregamento que usaram `ASAN_OPTIONS=detect_leaks=0`, ASan e UBSan permaneceram ativos, mas vazamentos não foram avaliados. Esse ajuste já é usado por parte dos testes CMake do projeto. Os testes de Mario/Link foram repetidos fora do sandbox e passaram sem essa falha de ambiente.

Nos probes isolados, foram extraídas as funções originais e o helper `copy_bytes`; os readers foram simulados e o alocador da arena foi substituído por `calloc` por objeto. Esse detalhe é essencial para interpretar a diferença entre o resultado desses probes e o carregamento normal dos assets.

Logs locais desta revisão, não versionados: `/tmp/melee-review-debug-build.log`, `/tmp/melee-review-debug-tests.log`, `/tmp/melee-review-sanitize-build.log`, `/tmp/melee-review-sanitize-tests.log`, `/tmp/melee-review-samus-probe.log`, `/tmp/melee-review-yoshi-probe.log` e `/tmp/melee-review-npm-play.log`. O log do CTest sanitize preserva as duas falhas iniciais de ambiente; o resultado da repetição externa é o registrado na tabela acima.

## Ordem sugerida de trabalho

1. Corrigir R01/R02 e adicionar detecção por objeto no alocador de diagnóstico.
2. Refazer a seleção/materialização de `yakumono_param` e restaurar a recusa explícita de layouts desconhecidos: R03–R05.
3. Corrigir profundidade e validar CPU/GPU: R06.
4. Corrigir aplicação de opções, eventos e ciclo de vida: R07–R12 e R14.
5. Corrigir o comando de execução e os critérios do teste aleatório: R13/R15.
6. Executar a matriz ampliada em Debug, Release e sanitize; atualizar os status públicos somente com o que foi comprovado.

## Cobertura do histórico

| Commit | Área revisada |
| --- | --- |
| `fa257ed6d` | Registro histórico de sanitizadores; não adiciona implementação |
| `1d30d447f` | Renderer, opções de vídeo, presets e testes de janela |
| `04480d38b` | Loop de cena e tratamento de ticks sem input |
| `8de587128` | Remoção de pacing externo e VSync |
| `59e34298d` | FPS e tradução da interface |
| `5b9473bb3` | Unlimited e pacing de apresentação |
| `f60740a8c` | Organização dos dados dos menus |
| `e45ca8e4f` | Tradutores de personagens/cenários, Ness, ferramentas e tipos |
| `5c619906c` | README do port |
| `cd8f479f5` | Refactor de atributos e animação de Yoshi |
| `6704efe5b` | README e posicionamento do projeto |
| `576cb7734` | ImGui, integração de eventos, unlock e scripts experimentais |
| `e40bc4c0f` | Texturas customizadas e bibliotecas incorporadas |
| `b40c4d13d` | Correções de compilação das texturas |
| `1d9c8c69e` | Descoberta recursiva de texturas |
| `86fa3ef36` | Dimensões das substituições |
| `7f45ba3da` | MSAA e anisotropia |
| `c704f182f` | Controles novos no ImGui |
| `5548569c0` | Compatibilidade de leitura e escrita das configurações |
| `f332ddd09` | Depth textures e atributos adicionais de vértice |
| `1348ec4a5` | AObj, Final Destination, ponteiros de texturas e UB em Release |

As substituições de acessos por offsets por campos nomeados nos menus/HUD, a preservação do ponteiro completo em `HSD_AObjDesc`, o uso de `memcpy` em `fabsf_bitwise` e a remoção das escritas em `sqrt_tmp - N` são avanços a preservar. Não foram identificadas regressões adicionais comprovadas nesses ajustes; a aprovação dessas partes permanece limitada aos testes executados, sem validação do build PowerPC ou de todos os modos do jogo.
