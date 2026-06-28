# Chess Engine

Work-In-Progress - Partially implemented, API subject to change.
A chess engine written in C++ with a Python 3 API.

## Build

### Release Build
```
mkdir release && cd release
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

### Debug Build
```
mkdir debug && cd debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```

## Use in Python 3

### Import
```
from release import libchess
# from debug import libchess # If you built debug
```

### Create a Board
```
# New board in start position
board = libchess.Board()

# New board from FEN string
board_from_fen = libchess.Board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1")
```

### Check for Errors
```
if board.has_error():
    error = board.get_error()
    print(error)
    exit(1)
```

### Search for Move
```
time_limit_ms = 1000 # Currently unimplemented, for reference only
best_move = board.go(time_limit_ms)
print(best_move) # Long Algebraic Notation e.g. e2e4
board.make_move(best_move)
```

### Print Board State
```
print(repr(board))
```
