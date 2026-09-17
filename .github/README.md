# Super Smash Bros. Melee PC Port

Este projeto é um port nativo de Super Smash Bros. Melee para PC (Windows e Linux). Inicialmente criado a partir de um fork do projeto de decompilação do Melee (`doldecomp/melee`), o foco deste repositório agora foi inteiramente redirecionado para a criação de um executável nativo do jogo, não necessitando de emuladores (como o Dolphin) ou execução de código PowerPC.

## Visão Geral

Enquanto o projeto de decompilação original tem como objetivo reconstruir um executável 1:1 (`main.dol`) idêntico ao de GameCube para documentação e mods, este port nativo recompila o código fonte em C (gameplay, personagens, menus) diretamente para plataformas modernas (como a arquitetura x86-64 do PC).

Para que isso seja possível, uma nova camada de abstração de plataforma está sendo desenvolvida para substituir as dependências diretas ao hardware e SDK do GameCube (como OS, GX, VI, DVD, PAD, CARD, AX, ARAM, DSP e THP) por equivalentes modernos (OpenGL/Vulkan, SDL, etc). O modelo arquitetural adotado é semelhante ao de projetos notáveis de decompilação como o *Ship of Harkinian*.

### Distribuição e Recursos Legais
- **Nenhum Asset Protegido:** O código-fonte presente neste repositório e os eventuais executáveis distribuídos **não** conterão assets originais de propriedade da Nintendo (modelos, áudios, texturas, etc).
- **Traga o seu próprio jogo (BYOG):** Para que o jogo funcione, o usuário precisará fornecer um dump (ISO/GCM) de uma cópia legítima de Super Smash Bros. Melee (versão alvo inicial: `GALE01` NTSC-U 1.02). Na primeira execução, o port irá extrair os arquivos e assets necessários para uma pasta local de recursos a fim de ser carregado pelo jogo.
- **Melhorias do Port PC:** Resoluções flexíveis e customizáveis, suporte direto para gamepads/teclado de PC via bibliotecas modernas e melhorias framerate.

## Documentação

Para informações detalhadas sobre a arquitetura do port, atualizações de progresso, plano para a liberação da versão de testes (MVP) e os principais desafios técnicos de portabilidade em relação à base de decompilação original, confira a pasta [`docs/`](docs/).

Recomendamos a leitura dos documentos principais do planejamento técnico:
- [Plano Inicial do Port Nativo PC](docs/native_pc_port_plan.md)
- [Status Atual do Port Nativo](docs/native_port_status.md)
- [Desenvolvimento do Port Nativo](docs/native_port_development.md)
- [Documento Base: Como Começar](docs/getting_started.md)

## Como Compilar

*(As instruções de compilação completas da versão PC para Windows/Linux estarão disponíveis assim que a infraestrutura de build estiver concluída nesta fase de transição.)*

---
**Aviso Legal:** *Este projeto é um esforço de código aberto, não oficial, feito por fãs, dedicado à preservação técnica e estudo de engenharia de software através da engenharia reversa do jogo original. Super Smash Bros. Melee e seus respectivos assets, designs e nomes são propriedades registradas e intelectuais da Nintendo.*
