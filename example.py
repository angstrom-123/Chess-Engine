#!/usr/bin/env python3

# Temporarily copy the package to the python path
import sys 
sys.path.insert(0, "./build")

import libchess

# board = libchess.Board()
board = libchess.Board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1")
if board.has_error():
    print("Error:", board.get_error())
    exit(0)

board.go(1000);

print(repr(board));
