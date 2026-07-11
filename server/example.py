#!/usr/bin/env python3

import libchess

# ========== Create a board ==========

# Start position
# board = libchess.Board()

# Alternatively Arbitrary position FEN
board = libchess.Board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1")

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
