import {
    BOARD_SIZE,
    Board,
    isWhite,
    MoveData,
    sameColor,
    Square,
    type Color,
    type OctalDigit,
    type Piece,
} from "./board.mjs";

export function getMoves(board: Board, square: number): number[] {
    var pseudoLegals: number[];

    switch (board.squares[square] as Piece) {
        case " ":
            return [];
        case "p":
        case "P":
            pseudoLegals = getPawnMoves(board, square);
            break;
        case "n":
        case "N":
            pseudoLegals = getKnightMoves(board, square);
            break;
        case "b":
        case "B":
            pseudoLegals = getBishopMoves(board, square);
            break;
        case "r":
        case "R":
            pseudoLegals = getRookMoves(board, square);
            break;
        case "q":
        case "Q":
            pseudoLegals = getQueenMoves(board, square);
            break;
        case "k":
        case "K":
            pseudoLegals = getKingMoves(board, square);
            break;
    }

    const enemy: Color = isWhite(board.squares[square] as Piece) ? "black" : "white";
    var legals: number[] = [];
    for (const move of pseudoLegals) {
        const moveData: MoveData = board.makeMove(Square.fromIndex(square), Square.fromIndex(move));

        const enemyAttacks: boolean[] = board.getAllAttacks(enemy);
        var isSafe: boolean = true;
        for (let i = 0; i < BOARD_SIZE * BOARD_SIZE; i++) {
            if (
                sameColor(moveData.piece, board.squares[i] as Piece) &&
                (board.squares[i] === "k" || board.squares[i] === "K") &&
                enemyAttacks[i]
            ) {
                isSafe = false;
                break;
            }
        }

        if (isSafe) {
            legals.push(move);
        }

        board.unmakeMove(moveData);
    }

    return legals;
}

export function getAttacks(board: Board, square: number): number[] {
    switch (board.squares[square] as Piece) {
        case " ":
            return [];
        case "p":
        case "P":
            return getPawnAttacks(board, square);
        case "n":
        case "N":
            return getKnightMoves(board, square);
        case "b":
        case "B":
            return getBishopMoves(board, square);
        case "r":
        case "R":
            return getRookMoves(board, square);
        case "q":
        case "Q":
            return getQueenMoves(board, square);
        case "k":
        case "K":
            return getKingAttacks(board, square);
    }
}

function getPawnMoves(board: Board, square: number) {
    const piece: Piece = board.squares[square]!;
    if (piece != "p" && piece != "P") {
        throw new Error("Expected pawn in square");
    }

    const moves: number[] = [];

    const isStartSquare: boolean =
        isWhite(piece) == !board.boardFlipped
            ? Math.floor(square / BOARD_SIZE) == 6
            : Math.floor(square / BOARD_SIZE) == 1;
    const direction: number = isWhite(piece) == !board.boardFlipped ? -1 : 1;

    const singlePush: number = square + direction * BOARD_SIZE;
    const doublePush: number = square + direction * BOARD_SIZE * 2;
    if (board.squares[singlePush] === " ") {
        moves.push(singlePush);
        if (board.squares[doublePush] === " " && isStartSquare) {
            moves.push(doublePush);
        }
    }

    const squareX: OctalDigit = (square % BOARD_SIZE) as OctalDigit;
    const squareY: OctalDigit = Math.floor(square / BOARD_SIZE) as OctalDigit;

    const offsets: number[][] = [
        [1, 1],
        [-1, 1],
    ];
    for (const offset of offsets) {
        const moveX: OctalDigit = (squareX + offset[0]! * direction) as OctalDigit;
        const moveY: OctalDigit = (squareY + offset[1]! * direction) as OctalDigit;
        if (moveX >= 0 && moveX < 8 && moveY >= 0 && moveY < 8) {
            const move: number = moveY * BOARD_SIZE + moveX;
            const movePiece: Piece = board.squares[move]!;
            if (
                (movePiece !== " " && !sameColor(piece, movePiece)) ||
                move === board.enPassantSquare
            ) {
                moves.push(move);
            }
        }
    }

    return moves;
}

function getPawnAttacks(board: Board, square: number): number[] {
    const piece: Piece = board.squares[square]!;
    if (piece != "p" && piece != "P") {
        throw new Error("Expected pawn in square");
    }

    const moves: number[] = [];

    const direction: number = isWhite(piece) == !board.boardFlipped ? -1 : 1;

    const squareX: OctalDigit = (square % BOARD_SIZE) as OctalDigit;
    const squareY: OctalDigit = Math.floor(square / BOARD_SIZE) as OctalDigit;

    const offsets: number[][] = [
        [1, 1],
        [-1, 1],
    ];
    for (const offset of offsets) {
        const moveX: OctalDigit = (squareX + offset[0]! * direction) as OctalDigit;
        const moveY: OctalDigit = (squareY + offset[1]! * direction) as OctalDigit;
        if (moveX >= 0 && moveX < 8 && moveY >= 0 && moveY < 8) {
            const move: number = moveY * BOARD_SIZE + moveX;
            const movePiece: Piece = board.squares[move]!;
            if (
                (movePiece !== " " && !sameColor(piece, movePiece)) ||
                move === board.enPassantSquare
            ) {
                moves.push(move);
            }
        }
    }

    return moves;
}

function getKnightMoves(board: Board, square: number): number[] {
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
            if (movePiece === " " || !sameColor(piece, movePiece)) {
                moves.push(move);
            }
        }
    }

    return moves;
}

function getKingMoves(board: Board, square: number): number[] {
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
            if (movePiece === " " || !sameColor(piece, movePiece)) {
                moves.push(move);
            }
        }
    }

    // Castling
    // Normal
    // | 0  |  1 |  2 |  3 |  4 |  5 |  6 |  7 |
    //
    // | 56 | 57 | 58 | 59 | 60 | 61 | 62 | 63 |
    //   R         ^^        K         ^^   R
    //
    // Flipped
    // | 0  |  1 |  2 |  3 |  4 |  5 |  6 |  7 |
    //
    // | 56 | 57 | 58 | 59 | 60 | 61 | 62 | 63 |
    //   R    ^^        K         ^^        R

    const enemyAttacks: boolean[] = board.getAllAttacks(isWhite(piece) ? "black" : "white");

    if (!board.boardFlipped) {
        if ((isWhite(piece) && board.whiteCanCastleLong) || (!isWhite(piece) && board.blackCanCastleLong)) {
            if (
                board.squares[square - 1] === " " &&
                board.squares[square - 2] === " " &&
                !enemyAttacks[square] &&
                !enemyAttacks[square - 1] &&
                !enemyAttacks[square - 2]
            ) {
                moves.push(square - 2);
            }
        }

        if ((isWhite(piece) && board.whiteCanCastleShort) || (!isWhite(piece) && board.blackCanCastleLong)) {
            if (
                board.squares[square + 1] === " " &&
                board.squares[square + 2] === " " &&
                !enemyAttacks[square] &&
                !enemyAttacks[square + 1] &&
                !enemyAttacks[square + 2]
            ) {
                moves.push(square + 2);
            }
        }
    } else {
        if ((isWhite(piece) && board.whiteCanCastleLong) || (!isWhite(piece) && board.blackCanCastleLong)) {
            if (
                board.squares[square + 1] === " " &&
                board.squares[square + 2] === " " &&
                !enemyAttacks[square] &&
                !enemyAttacks[square + 1] &&
                !enemyAttacks[square + 2]
            ) {
                moves.push(square + 2);
            }
        }

        if ((isWhite(piece) && board.whiteCanCastleShort) || (!isWhite(piece) && board.blackCanCastleLong)) {
            if (
                board.squares[square - 1] === " " &&
                board.squares[square - 2] === " " &&
                !enemyAttacks[square] &&
                !enemyAttacks[square - 1] &&
                !enemyAttacks[square - 2]
            ) {
                moves.push(square - 2);
            }
        }
    }

    // if (isWhite(piece)) {
    //     const king: number = !board.boardFlipped ? 60 : 3;
    //     if (
    //         board.whiteCanCastleLong &&
    //         board.squares[king - 1] === " " &&
    //         board.squares[king - 2] === " " &&
    //         !enemyAttacks[king] &&
    //         !enemyAttacks[king - 1] &&
    //         !enemyAttacks[king - 2]
    //     ) {
    //         moves.push(king - 2);
    //     }
    //
    //     if (
    //         board.whiteCanCastleShort &&
    //         board.squares[king + 1] === " " &&
    //         board.squares[king + 2] === " " &&
    //         !enemyAttacks[king] &&
    //         !enemyAttacks[king + 1] &&
    //         !enemyAttacks[king + 2]
    //     ) {
    //         moves.push(king + 2);
    //     }
    // } else {
    //     const king: number = !board.boardFlipped ? 4 : 59;
    //     if (
    //         board.blackCanCastleLong &&
    //         board.squares[king - 1] === " " &&
    //         board.squares[king - 2] === " " &&
    //         !enemyAttacks[king] &&
    //         !enemyAttacks[king - 1] &&
    //         !enemyAttacks[king - 2]
    //     ) {
    //         moves.push(king - 2);
    //     }
    //
    //     if (
    //         board.blackCanCastleShort &&
    //         board.squares[king + 1] === " " &&
    //         board.squares[king + 2] === " " &&
    //         !enemyAttacks[king] &&
    //         !enemyAttacks[king + 1] &&
    //         !enemyAttacks[king + 2]
    //     ) {
    //         moves.push(king + 2);
    //     }
    // }

    return moves;
}

function getKingAttacks(board: Board, square: number): number[] {
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
            if (movePiece === " " || !sameColor(piece, movePiece)) {
                moves.push(move);
            }
        }
    }

    return moves;
}

function getBishopMoves(board: Board, square: number): number[] {
    const piece: Piece = board.squares[square]!;
    if (piece != "b" && piece != "B") {
        throw new Error("Expected bishop in square");
    }
    return getSliderMoves(board, square, false, true);
}

function getRookMoves(board: Board, square: number): number[] {
    const piece: Piece = board.squares[square]!;
    if (piece != "r" && piece != "R") {
        throw new Error("Expected rook in square");
    }
    return getSliderMoves(board, square, true, false);
}

function getQueenMoves(board: Board, square: number): number[] {
    const piece: Piece = board.squares[square]!;
    if (piece != "q" && piece != "Q") {
        throw new Error("Expected queen in square");
    }
    return getSliderMoves(board, square, true, true);
}

function getSliderMoves(
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
            } else if (sameColor(piece, movePiece)) {
                break;
            } else {
                moves.push(move);
                break;
            }
        }
    }

    return moves;
}
