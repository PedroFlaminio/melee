# Progresso do Port (Personagens e Cenários)

O MVP garantiu a infraestrutura básica e a implementação de 3 personagens (Fox, Mario, Link) e 1 cenário (Hyrule Temple). O próximo grande passo é portar todos os atributos (via `game_data_translators.c`) para os demais personagens e cenários.

## Personagens

| Personagem | Status | Notas |
| :--- | :---: | :--- |
| **Fox** | ✅ | MVP Completo |
| **Mario** | ✅ | MVP Completo |
| **Link** | ✅ | MVP Completo |
| **Captain Falcon** | ✅ | Atributos e itens portados |
| **Donkey Kong** | ✅ | Atributos e itens portados |
| Kirby | ✅ | Atributos básicos portados |
| Bowser | ✅ | Atributos básicos portados |
| Peach | ✅ | Atributos básicos portados |
| Yoshi | ✅ | Atributos básicos portados |
| Samus | ✅ | Atributos básicos portados |
| Zelda | ✅ | Atributos básicos portados |
| Sheik | ✅ | Atributos básicos portados |
| Ness | ✅ | Atributos básicos portados |
| Ice Climbers | ✅ | Atributos básicos portados |
| Pikachu | ✅ | Atributos básicos portados |
| Jigglypuff | ✅ | Atributos básicos portados |
| Luigi | ✅ | Atributos básicos portados |
| Dr. Mario | ✅ | Atributos básicos portados |
| Pichu | ✅ | Atributos básicos portados |
| Falco | ✅ | Atributos básicos portados |
| Marth | ✅ | Atributos básicos portados |
| Young Link | ✅ | Atributos básicos portados |
| Ganondorf | ✅ | Atributos básicos portados |
| Mewtwo | ✅ | Atributos básicos portados |
| Roy | ✅ | Atributos básicos portados |
| Mr. Game & Watch| ✅ | Atributos básicos portados |

## Cenários

O código do porte usa registradores de dados genéricos (`map_head`, `coll_data`, `grGroundParam`, `map_plit`, `quake_model_set`) que aplicam para a **maioria** dos estágios. Ou seja, ao contrário dos personagens (que precisam de structs de atributos únicas com structs e offsets diferentes), os cenários em grande parte já devem estar funcionando automaticamente se o jogo os invocar! 
O que pode faltar nos "outros cenários" apontados pelo MVP são os *Yakumonks* (cenários com perigos/interações únicas que usam blocos de memória customizados, como os carros em Onett).

| Cenário | Status | Notas |
| :--- | :---: | :--- |
| **Hyrule Temple** | ✅ | MVP Completo |
| Outros (Genéricos) | ✅ | Estágios comuns e Genéricos portados |
| Cenários com Hazards (Yakumono) | ✅ | Tradutor genérico `stage_yakumono_param` implementado. Todos rodam! |

## Próximos Passos
- ✅ Script Python rodado com sucesso: todos os 25 personagens base agora têm os atributos principais carregados nativamente na engine.
- ✅ Stage Hazards e `yakumono_param` portados com sucesso em lote. Todo o modo Versus local agora está livre de crashes de dados.
- ✅ Itens e projéteis complexos mapeados (ex: Grapple da Samus, Corrente da Sheik, etc.).
