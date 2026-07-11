# Chess Engine

Work-In-Progress - Partially implemented, API subject to change.
A chess engine written in C++ with a Python 3 API.

## Prerequisites
- Modern C++ compiler (>=c++23)
- Python 3.1x
- Astral uv
- npm
- CMake
- make / windows alternative (c++ build system)

## Build and Run Client

### Build library
```shell
mkdir build && cs build 
cmake .. -DCMAKE_BUILD_TYPE=Release 
make
cd ..
```

### Set up Server
```shell
uv sync
```

On Linux Run:
```shell
source .venv/bin/activate
```
On Windows Run: 
```shell
.\venv\Scripts\Activate.ps1
```

### Set up client
```shell
npm install 
```

### Start the server
```shell
npm run start
```

### Done 
Your server should be running at http://localhost:8000 (by default)

## Development

### Debug Library Build
```shell
mkdir build && cd build 
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```

### Rebuilding Library
If you ever rebuild the c++ library, you need to refresh uv dependencies.
```shell
uv pip install -e . --force-reinstall
```

### Development Server
Note that typescript files do not currently auto-refresh, so you need to rerun the dev server if you change those. Changes to the server-side should be fine.
```shell
npm run dev
```

### Library Python Code Example
```python
import libchess

# ========== Create a board ==========

# Start position
board = libchess.Board()

# Alternatively Arbitrary position FEN
# board = libchess.Board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1")

# ========== Check for errors ==========

if board.has_error():
    error = board.get_error()
    print(error)
    exit(1)

# ========== Search for best move ==========

# Time limit currently unimplemented, for reference only
time_limit_ms = 1000

# Long algebraic notation (e.g. e2e4)
best_move = board.go(time_limit_ms)
print(f"Best Move: {best_move}")

# ========== Update Board State ==========

print(f"\nBefore:\n\n{repr(board)}")

board.make_move(best_move)

print(f"\nAfter:\n\n{repr(board)}")
```
