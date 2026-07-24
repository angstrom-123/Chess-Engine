from typing_extensions import override

from server.engine import BaseEngine

from libchess import Board

class Jupiter(BaseEngine):
    name: str = "Jupiter"
    board: Board | None = None 

    @override
    def init(self, fen: str | None = None) -> None:
        self.board = Board() if fen is None else Board(fen)

        if self.board.has_error():
            raise RuntimeError(f"[JUPITER] Error while initialising board: {self.board.get_error()}\nFEN: {fen}")

    @override
    def go(self, msLeft: int) -> str:
        if self.board is None:
            raise AttributeError("self.board is not initialised. Try calling init.")

        move: str = self.board.go(msLeft)
        if self.board.has_error():
            raise RuntimeError(f"[JUPITER] Error while searching for move: {self.board.get_error()}\n{repr(self.board)}")

        return move

    @override
    def move(self, move: str) -> None:
        if self.board is None:
            raise AttributeError("[JUPITER] self.board is not initialised. Try calling init.")

        self.board.make_move(move)
        if self.board.has_error():
            raise RuntimeError(f"[JUPITER] Error while making move {move}: {self.board.get_error()}\n{repr(self.board)}")
