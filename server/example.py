#!/usr/bin/env python3

# import libchess
from libchess import Board

# ========== Create a board ==========

# Start position
# board = libchess.Board()

# Alternatively Arbitrary position FEN
board: Board = Board("6k1/p7/1p6/8/7P/5K2/8/6q1 w - - 0 1")

# ========== Check for errors ==========

if board.has_error():
    error = board.get_error()
    print(error)
    exit(1)

# ========== Search for best move ==========

# 10 seconds left
time_remaining_ms: int = 10000

# Long algebraic notation (e.g. e2e4)
best_move: str = board.go(time_remaining_ms)
print(f"Best Move: {best_move}")

# ========== Update Board State ==========

print(f"\nBefore:\n\n{repr(board)}")

board.make_move(best_move)

print(f"\nAfter:\n\n{repr(board)}")
