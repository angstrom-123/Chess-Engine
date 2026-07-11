export declare const BOARD_SIZE = 8;
export declare const PIECE_SCALE = 0.73;
export declare const ROOK_SCALE = 0.68;
export declare const LIGHT_SQUARE_HEX = "#d4c49e";
export declare const LIGHT_SQUARE_HIGHLIGHT_HEX = "#c35858";
export declare const LIGHT_SQUARE_SELECTED_HEX = "#fffd83";
export declare const DARK_SQUARE_HEX = "#8a6c45";
export declare const DARK_SQUARE_HIGHLIGHT_HEX = "#a34636";
export declare const DARK_SQUARE_SELECTED_HEX = "#fffd83";
export type OctalDigit = 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7;
export declare const Colors: readonly ["none", "white", "black"];
export type Color = (typeof Colors)[number];
export declare const Pieces: readonly [" ", "P", "p", "N", "n", "B", "b", "R", "r", "Q", "q", "K", "k"];
export type Piece = (typeof Pieces)[number];
export declare class Square {
    x: OctalDigit;
    y: OctalDigit;
    constructor(x: OctalDigit, y: OctalDigit);
    AsIndex(): number;
    Equals(square: Square): boolean;
}
export declare function SameColor(piece0: Piece, piece1: Piece): boolean;
export declare function IsWhite(piece: Piece): boolean;
export declare class Board {
    squares: Piece[];
    isWhite: boolean;
    enPassantSquare: number;
    whiteCanCastleLong: boolean;
    whiteCanCastleShort: boolean;
    blackCanCastleLong: boolean;
    blackCanCastleShort: boolean;
    whiteTurn: boolean;
    private selectedSquare;
    private draggingPiece;
    private hideSprite;
    private sprites;
    private spritesLoaded;
    private boardCanvas;
    private spriteCanvas;
    private boardCtx;
    private spriteCtx;
    private whiteTime;
    private blackTime;
    private increment;
    constructor();
    Init(color: Color, time: number, increment: number, squares: Piece[]): Promise<void>;
    LoadSprites(): Promise<void>;
    DrawSpriteAt(piece: Piece, x: OctalDigit, y: OctalDigit): void;
    OnMouseDown(e: MouseEvent): void;
    OnMouseUp(e: MouseEvent): void;
    OnMouseLeave(_e: MouseEvent): void;
    OnMouseMove(e: MouseEvent): void;
    MakeMove(from: Square, to: Square): void;
    DrawBoard(highlight?: number[]): void;
    DrawPieces(): void;
}
//# sourceMappingURL=board.d.mts.map