# Plano de video e apresentacao

## Objetivo

Separar a simulacao original do Melee (60 Hz) da apresentacao, mantendo a
primeira como fonte de verdade e oferecendo configuracao de aspecto,
resolucao/upscaling e frequencia de apresentacao sem alterar frame data,
fisica, inputs ou determinismo.

## Estado atual (implementado)

O menu Esc agora oferece cinco opcoes funcionais inspiradas no Ship of Harkinian:

1. **RESOLUCAO** — Multiplicador de resolucao interna: 1x (nativo 640x528),
   2x (1280x1056), 3x (1920x1584), 4x (2560x2112), 5x (3200x2640).
   O framebuffer de render e recriado dinamicamente; o jogo rasteriza em
   alta resolucao sem esticar, e a imagem composta na janela preserva
   aspecto com letterbox/pillarbox.

2. **ASPECTO** — 4:3 (original), 16:9, 21:9. Aplicado via calculo de
   letterbox/pillarbox no blit para a janela. A projecao e logica do jogo
   nao mudam.

3. **FILTRO** — Nearest (pixel-perfect) ou Linear (bilinear). Aplicado no
   glBlitFramebuffer a cada quadro.

4. **JANELA** — Janela, Tela Cheia (SDL fullscreen) ou Sem Bordas
   (borderless maximized). Aplicado imediatamente via SDL APIs e restaurado
   ao reabrir o jogo.

5. **FREQUENCIA** — 60, 120, 144, 165 ou 240 FPS. Preparado para
   interpolacao de pose visual futura; a simulacao permanece 60 Hz.

Todas as configuracoes persistem em `~/.local/share/MeleePC/MeleePC/video-settings.txt`.

## Ordem de implementacao futura

1. Medir CPU de simulacao, rasterizacao, composicao e present separadamente,
   mostrando FPS efetivo e o gargalo no menu.
2. Guardar poses visuais anterior/atual e implementar interpolacao para os
   limites de 120, 144, 165 e 240 FPS. Cortes de camera, teleporte, spawn,
   troca de cena e efeitos sem dados interpolaveis devem manter a pose valida.
3. Validar cada modo contra 60 Hz: testes de simulacao e inputs devem ser
   identicos; capturas consecutivas devem provar que os quadros extras sao
   distintos e que nenhum modo degrada abaixo do limite escolhido.
4. Suporte a widescreen HOR+ no campo de visao da camera (como SoH faz),
   expandindo o FOV horizontal sem esticar a imagem.

## Criterios de aceite

- Abrir o menu nao reduz a taxa de apresentacao mensuravelmente.
- Aspecto, resolucao e filtro alteram a imagem de forma observavel e persistem.
- 60 FPS continua sendo o comportamento original; os limites acima dele nao
  executam ticks extras do jogo.
- A mudanca de configuracao nao perde input nem reinicia a partida.
- Trocar para tela cheia ou sem bordas funciona sem flicker nem crash.
