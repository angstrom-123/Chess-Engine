#!/usr/bin/env python3

# import libchess
from libchess import Board

# ========== Create a board ==========

# Start position
# board = libchess.Board()

# Alternatively Arbitrary position FEN
board: Board = Board("k2K4/8/8/8/8/8/2p5/8 b KQkq - 11 6")

# ========== Check for errors ==========

if board.has_error():
    error = board.get_error()
    print(error)
    exit(1)

# ========== Search for best move ==========

# Time limit currently unimplemented, for reference only
time_limit_ms: int = 1000

# Long algebraic notation (e.g. e2e4)
best_move: str = board.go(time_limit_ms)
print(f"Best Move: {best_move}")

# ========== Update Board State ==========

print(f"\nBefore:\n\n{repr(board)}")

board.make_move(best_move)

print(f"\nAfter:\n\n{repr(board)}")
