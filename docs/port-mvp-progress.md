# Progresso do MVP do port nativo

## Definição de MVP

Fluxo local completo: iniciar o executável, navegar de título para VS, escolher
dois jogadores e um estágio, e concluir uma luta local com vídeo, entrada,
áudio e resultado funcional.

## Linha de base — 13 de setembro de 2026

**Estimativa: 35% (faixa de confiança: 30–40%).**

| Área | Estado | Peso no MVP |
| --- | --- | --- |
| Plataforma host (memória, relógio, DVD virtual, input) | funcional e testada | 15% |
| Assets e renderização HSD/GX | funcional para cenas/modelos selecionados | 20% |
| Inicialização, título e menu principal | título e rota até o menu validados | 15% |
| Configuração de VS | regras e estado inicial 2P disponíveis, sem fluxo visual completo | 10% |
| Luta (fighters, stage, colisão, câmera, HUD, KO) | não integrada | 30% |
| Áudio, distribuição e regressão end-to-end | parcial; sem partida validada | 10% |

### Evidências verificadas

- `ctest --test-dir build/host --output-on-failure`: 13/13 testes aprovados.
- A cena de título, animações e a transição para o menu principal possuem testes
  com assets locais.
- `--diagnose-local-match` materializa dados de duas pessoas, regras e estágio,
  mas ainda não inicia a cena de combate.
- A rota com input chega a `GM_VS (0x02)`; esse modo e sua cena ainda não fazem
  parte da tabela do host. Seleção de personagem, seleção de estágio e runtime
  da luta permanecem fora do fluxo.

## Próximo marco

Chegar a uma primeira luta local determinística com dois personagens e um
estágio fixos. A seguir, substituir os valores fixos pelo fluxo de seleção.

## Registro de atualizações

| Data | Percentual | Mudança/evidência |
| --- | ---: | --- |
| 2026-09-13 | 25% | Linha de base criada; infraestrutura, título e menu parcial validados. |
| 2026-09-13 | 25% | UI de memory card isolada no host; rota validada até `GM_VS (0x02)`. |
| 2026-09-13 | 25% | CSS, SSS e regras de menu passaram a compilar/linkar no core host; falta conectá-los e executá-los. |
| 2026-09-13 | 25% | `GM_VS`, CSS e SSS registrados na tabela host. CSS inicia e para explicitamente no tradutor ausente de `MnSelectChrDataTable`; não há crash silencioso. |
| 2026-09-13 | 30% | `MnSelectChrDataTable` passou a materializar câmera, luzes, fog e nove modelos CSS no layout host; carga verificada com `MnSlChr.usd`. |
| 2026-09-13 | 35% | `MnSelectStageDataTable` passou a materializar câmera, luzes, fog, 11 modelos e o modelo aleatório SSS; carga verificada com `MnSlMap.usd`. |
