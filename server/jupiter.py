from server.engine import BaseEngine
from libchess import Board
from typing_extensions import override

class Jupiter(BaseEngine):
    name: str = "Jupiter"
    board: Board | None = None 

    @override
    def init(self, fen: str | None = None) -> None:
        if fen is None:
            self.board = Board()
        else:
            self.board = Board(fen)

    @override
    def go(self, msLeft: int) -> str:
        if self.board is not None:
            return self.board.go(msLeft)
        else:
            raise AttributeError("self.board is not initialised. Try calling init.")

    @override
    def move(self, move: str) -> None:
        if self.board is not None:
            self.board.make_move(move)
        else:
            raise AttributeError("self.board is not initialised. Try calling init.")
