import { BOARD_SIZE, Board, IsWhite, SameColor } from "./board.mjs";
export function GetMoves(board, square) {
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
function GetPawnMoves(board, square) {
    const piece = board.squares[square];
    if (piece != "p" && piece != "P") {
        throw new Error("Expected pawn in square");
    }
    const moves = [];
    const isStartSquare = IsWhite(piece) == board.isWhite
        ? Math.floor(square / BOARD_SIZE) == 6
        : Math.floor(square / BOARD_SIZE) == 1;
    const direction = IsWhite(piece) == board.isWhite ? -1 : 1;
    const singlePush = square + direction * BOARD_SIZE;
    const doublePush = square + direction * BOARD_SIZE * 2;
    if (board.squares[singlePush] === " ") {
        moves.push(singlePush);
        if (board.squares[doublePush] === " " && isStartSquare) {
            moves.push(doublePush);
        }
    }
    const possibleCaptures = [
        square + direction * BOARD_SIZE - 1,
        square + direction * BOARD_SIZE + 1,
    ];
    for (const capture of possibleCaptures) {
        const capturePiece = board.squares[capture];
        if (capture >= 0 &&
            capture < BOARD_SIZE * BOARD_SIZE &&
            ((capturePiece !== " " && !SameColor(piece, capturePiece)) ||
                capture == board.enPassantSquare)) {
            moves.push(capture);
        }
    }
    return moves;
}
function GetKnightMoves(board, square) {
    const piece = board.squares[square];
    if (piece != "n" && piece != "N") {
        throw new Error("Expected knight in square");
    }
    const moves = [];
    const squareX = (square % BOARD_SIZE);
    const squareY = Math.floor(square / BOARD_SIZE);
    const offsets = [
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
        const moveX = (squareX + offset[0]);
        const moveY = (squareY + offset[1]);
        if (moveX >= 0 && moveX < 8 && moveY >= 0 && moveY < 8) {
            const move = moveY * BOARD_SIZE + moveX;
            const movePiece = board.squares[move];
            if (movePiece === " " || !SameColor(piece, movePiece)) {
                moves.push(move);
            }
        }
    }
    return moves;
}
function GetKingMoves(board, square) {
    const piece = board.squares[square];
    if (piece != "k" && piece != "K") {
        throw new Error("Expected king in square");
    }
    const moves = [];
    const squareX = (square % BOARD_SIZE);
    const squareY = Math.floor(square / BOARD_SIZE);
    const offsets = [
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
        const moveX = (squareX + offset[0]);
        const moveY = (squareY + offset[1]);
        if (moveX >= 0 && moveX < 8 && moveY >= 0 && moveY < 8) {
            const move = moveY * BOARD_SIZE + moveX;
            const movePiece = board.squares[move];
            if (movePiece === " " || !SameColor(piece, movePiece)) {
                moves.push(move);
            }
        }
    }
    // TODO: Castling
    return moves;
}
function GetBishopMoves(board, square) {
    const piece = board.squares[square];
    if (piece != "b" && piece != "B") {
        throw new Error("Expected bishop in square");
    }
    return GetSliderMoves(board, square, false, true);
}
function GetRookMoves(board, square) {
    const piece = board.squares[square];
    if (piece != "r" && piece != "R") {
        throw new Error("Expected rook in square");
    }
    return GetSliderMoves(board, square, true, false);
}
function GetQueenMoves(board, square) {
    const piece = board.squares[square];
    if (piece != "q" && piece != "Q") {
        throw new Error("Expected queen in square");
    }
    return GetSliderMoves(board, square, true, true);
}
function GetSliderMoves(board, square, orthogonal, diagonal) {
    const piece = board.squares[square];
    const offsets = [
        [1, 1],
        [1, -1],
        [-1, 1],
        [-1, -1],
        [1, 0],
        [-1, 0],
        [0, 1],
        [0, -1],
    ];
    const moves = [];
    const startIndex = diagonal ? 0 : 4;
    const endIndex = orthogonal ? 8 : 4;
    const squareX = (square % BOARD_SIZE);
    const squareY = Math.floor(square / BOARD_SIZE);
    for (let i = startIndex; i < endIndex; i++) {
        for (let distance = 1; distance < 8; distance++) {
            const moveX = (squareX + offsets[i][0] * distance);
            const moveY = (squareY + offsets[i][1] * distance);
            if (moveX < 0 || moveX > 7 || moveY < 0 || moveY > 7) {
                break;
            }
            const move = moveY * BOARD_SIZE + moveX;
            const movePiece = board.squares[move];
            if (movePiece === " ") {
                moves.push(move);
            }
            else if (SameColor(piece, movePiece)) {
                break;
            }
            else {
                moves.push(move);
                break;
            }
        }
    }
    return moves;
}
//# sourceMappingURL=moves.mjs.map