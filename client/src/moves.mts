import { BOARD_SIZE, Board, IsWhite, SameColor, type OctalDigit, type Piece } from "./board.mjs";

export function GetMoves(board: Board, square: number): number[] {
    switch (board.squares[square]) {
        case " ":
            return [];
        case "p":
        case "P":
            return GetPawnMoves(board, square);
        case "n":
        case "N":
            return GetKnightMoves(board, square);
        case "b":
        case "B":
            return GetBishopMoves(board, square);
        case "r":
        case "R":
            return GetRookMoves(board, square);
        case "q":
        case "Q":
            return GetQueenMoves(board, square);
        case "k":
        case "K":
            return GetKingMoves(board, square);
    }
    throw new Error("Getting moves for unexpected piece");
}

function GetPawnMoves(board: Board, square: number) {
    const piece: Piece = board.squares[square]!;
    if (piece != "p" && piece != "P") {
        throw new Error("Expected pawn in square");
    }

    const moves: number[] = [];

    const isStartSquare: boolean =
        IsWhite(piece) == board.isWhite
            ? Math.floor(square / BOARD_SIZE) == 6
            : Math.floor(square / BOARD_SIZE) == 1;
    const direction: number = IsWhite(piece) == board.isWhite ? -1 : 1;

    const singlePush: number = square + direction * BOARD_SIZE;
    const doublePush: number = square + direction * BOARD_SIZE * 2;
    if (board.squares[singlePush] === " ") {
        moves.push(singlePush);
        if (board.squares[doublePush] === " " && isStartSquare) {
            moves.push(doublePush);
        }
    }

    const possibleCaptures: number[] = [
        square + direction * BOARD_SIZE - 1,
        square + direction * BOARD_SIZE + 1,
    ];
    for (const capture of possibleCaptures) {
        const capturePiece: Piece = board.squares[capture]!;
        if (
            capture >= 0 &&
            capture < BOARD_SIZE * BOARD_SIZE &&
            ((capturePiece !== " " && !SameColor(piece, capturePiece)) ||
                capture == board.enPassantSquare)
        ) {
            moves.push(capture);
        }
    }

    return moves;
}

function GetKnightMoves(board: Board, square: number): number[] {
    const piece: Piece = board.squares[square]!;
    if (piece != "n" && piece != "N") {
        throw new Error("Expected knight in square");
    }

    const moves: number[] = [];

    const squareX: OctalDigit = (square % BOARD_SIZE) as OctalDigit;
    const squareY: OctalDigit = Math.floor(square / BOARD_SIZE) as OctalDigit;

    const offsets: number[][] = [
        [1, 2],
        [2, 1],
        [2, -1],
        [1, -2],
        [-1, 2],
        [-2, 1],
        [-2, -1],
        [-1, -2],
    ];
    for (const offset of offsets) {
        const moveX: OctalDigit = (squareX + offset[0]!) as OctalDigit;
        const moveY: OctalDigit = (squareY + offset[1]!) as OctalDigit;
        if (moveX >= 0 && moveX < 8 && moveY >= 0 && moveY < 8) {
            const move: number = moveY * BOARD_SIZE + moveX;
            const movePiece: Piece = board.squares[move]!;
            if (movePiece === " " || !SameColor(piece, movePiece)) {
                moves.push(move);
            }
        }
    }

    return moves;
}

function GetKingMoves(board: Board, square: number): number[] {
    const piece: Piece = board.squares[square]!;
    if (piece != "k" && piece != "K") {
        throw new Error("Expected king in square");
    }

    const moves: number[] = [];

    const squareX: OctalDigit = (square % BOARD_SIZE) as OctalDigit;
    const squareY: OctalDigit = Math.floor(square / BOARD_SIZE) as OctalDigit;

    const offsets: number[][] = [
        [1, 1],
        [1, 0],
        [1, -1],
        [0, 1],
        [0, -1],
        [-1, 1],
        [-1, 0],
        [-1, -1],
    ];
    for (const offset of offsets) {
        const moveX: OctalDigit = (squareX + offset[0]!) as OctalDigit;
        const moveY: OctalDigit = (squareY + offset[1]!) as OctalDigit;
        if (moveX >= 0 && moveX < 8 && moveY >= 0 && moveY < 8) {
            const move: number = moveY * BOARD_SIZE + moveX;
            const movePiece: Piece = board.squares[move]!;
            if (movePiece === " " || !SameColor(piece, movePiece)) {
                moves.push(move);
            }
        }
    }

    // TODO: Castling

    return moves;
}

function GetBishopMoves(board: Board, square: number): number[] {
    const piece: Piece = board.squares[square]!;
    if (piece != "b" && piece != "B") {
        throw new Error("Expected bishop in square");
    }
    return GetSliderMoves(board, square, false, true);
}

function GetRookMoves(board: Board, square: number): number[] {
    const piece: Piece = board.squares[square]!;
    if (piece != "r" && piece != "R") {
        throw new Error("Expected rook in square");
    }
    return GetSliderMoves(board, square, true, false);
}

function GetQueenMoves(board: Board, square: number): number[] {
    const piece: Piece = board.squares[square]!;
    if (piece != "q" && piece != "Q") {
        throw new Error("Expected queen in square");
    }
    return GetSliderMoves(board, square, true, true);
}

function GetSliderMoves(
    board: Board,
    square: number,
    orthogonal: boolean,
    diagonal: boolean,
): number[] {
    const piece: Piece = board.squares[square]!;
    const offsets: number[][] = [
        [1, 1],
        [1, -1],
        [-1, 1],
        [-1, -1],
        [1, 0],
        [-1, 0],
        [0, 1],
        [0, -1],
    ];

    const moves: number[] = [];

    const startIndex: number = diagonal ? 0 : 4;
    const endIndex: number = orthogonal ? 8 : 4;

    const squareX: OctalDigit = (square % BOARD_SIZE) as OctalDigit;
    const squareY: OctalDigit = Math.floor(square / BOARD_SIZE) as OctalDigit;

    for (let i: number = startIndex; i < endIndex; i++) {
        for (let distance: number = 1; distance < 8; distance++) {
            const moveX: OctalDigit = (squareX + offsets[i]![0]! * distance) as OctalDigit;
            const moveY: OctalDigit = (squareY + offsets[i]![1]! * distance) as OctalDigit;
            if (moveX < 0 || moveX > 7 || moveY < 0 || moveY > 7) {
                break;
            }

            const move: number = moveY * BOARD_SIZE + moveX;
            const movePiece: Piece = board.squares[move]!;

            if (movePiece === " ") {
                moves.push(move);
            } else if (SameColor(piece, movePiece)) {
                break;
            } else {
                moves.push(move);
                break;
            }
        }
    }

    return moves;
}
