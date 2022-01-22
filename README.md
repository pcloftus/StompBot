# StompBot
**Approximately 1600 ELO chess engine for play against human players or itself**  
*Patrick Loftus*  
*Last Updated: 01/18/22*

## In Action
![Gif Showcase](https://github.com/pcloftus/StompBot/blob/main/StompBot_showcase)

## Functionality
- 0x88 board representation and transposition table based on Zobrist hashing ([board_util.cc](./board_util.cc), [tt.cc](./tt.cc))
- Piece-square evaluation tables ([eval.cc](./eval.cc), [eval_init.cc](./eval_init.cc))
  - King tropism evaluation
  - BLIND implementation of Static Exchange Evaluation ([attacks.cc](./attacks.cc), [blind.cc](./blind.cc))
- Alpha-beta search using Negamax framework ([search.cc](./search.cc))
  - Principal Variation Search
  - Hash moves
  - Quiescence search
- Basic command-line interface and graphics ([cli.cc](./cli.cc))
  - With support for making engine moves for either side
  - Entering moves in algebraic notation
  - Loading a position from a FEN
