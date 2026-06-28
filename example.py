#!/usr/bin/env python3

from release import libchess
# from debug import libchess

board = libchess.Board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1")
if board.has_error():
    print("Error:", board.get_error())
    exit(1)

print("Starting State");
print(repr(board));
print();

move = board.go(1000);
board.make_move(move);
print(f"move: {move}")

print();
print(repr(board));
print();
