import * as movegen from "./moves.mjs";

export const BOARD_SIZE = 8;
export const PIECE_SCALE = 0.73;
export const ROOK_SCALE = 0.68;
export const LIGHT_SQUARE_HEX = "#d4c49e";
export const LIGHT_SQUARE_HIGHLIGHT_HEX = "#c35858";
export const LIGHT_SQUARE_SELECTED_HEX = "#fffd83";
export const DARK_SQUARE_HEX = "#8a6c45";
export const DARK_SQUARE_HIGHLIGHT_HEX = "#a34636";
export const DARK_SQUARE_SELECTED_HEX = "#fffd83";

export type OctalDigit = 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7;

export const Colors = ["none", "white", "black"] as const;
export type Color = (typeof Colors)[number];

export const Pieces = [" ", "P", "p", "N", "n", "B", "b", "R", "r", "Q", "q", "K", "k"] as const;
export type Piece = (typeof Pieces)[number];

export class Square {
    public x: OctalDigit;
    public y: OctalDigit;

    constructor(x: OctalDigit, y: OctalDigit) {
        this.x = x;
        this.y = y;
    }

    AsIndex(): number {
        return this.y * BOARD_SIZE + this.x;
    }

    Equals(square: Square): boolean {
        return square.x === this.x && square.y === this.y;
    }
}

export function SameColor(piece0: Piece, piece1: Piece) {
    return IsWhite(piece0) === IsWhite(piece1);
}

export function IsWhite(piece: Piece) {
    return piece.charCodeAt(0) < 91;
}

export class Board {
    public squares: Piece[];
    public isWhite: boolean = true;
    public enPassantSquare: number = -1;
    public whiteCanCastleLong: boolean = true;
    public whiteCanCastleShort: boolean = true;
    public blackCanCastleLong: boolean = true;
    public blackCanCastleShort: boolean = true;
    public whiteTurn: boolean = true;

    private selectedSquare: Square | null = null;
    private draggingPiece: boolean = false;
    private hideSprite: number = -1;
    private sprites: Map<Piece, HTMLImageElement>;
    private spritesLoaded: boolean = false;
    private boardCanvas: HTMLCanvasElement;
    private spriteCanvas: HTMLCanvasElement;
    private boardCtx: CanvasRenderingContext2D;
    private spriteCtx: CanvasRenderingContext2D;

    private whiteTime: number = 0;
    private blackTime: number = 0;
    private increment: number = 0;

    constructor() {
        this.squares = new Array<Piece>(64) as Piece[];
        this.sprites = new Map<Piece, HTMLImageElement>();

        const boardElement: Element | undefined =
            document.getElementsByClassName("board-canvas")[0];
        const spriteElement: Element | undefined =
            document.getElementsByClassName("sprite-canvas")[0];
        if (
            !(boardElement instanceof HTMLCanvasElement) ||
            !(spriteElement instanceof HTMLCanvasElement)
        ) {
            throw new Error("board-canvas or sprite-canvas is not a html canvas element");
        }
        this.boardCanvas = boardElement;
        this.spriteCanvas = spriteElement;

        const boardContext: CanvasRenderingContext2D | null = this.boardCanvas.getContext("2d");
        const spriteContext: CanvasRenderingContext2D | null = this.spriteCanvas.getContext("2d");
        if (boardContext === null || spriteContext === null) {
            throw new Error(
                "Could not obtain rendering context for board canvas nor sprite canvas",
            );
        }
        this.boardCtx = boardContext;
        this.spriteCtx = spriteContext;

        this.spriteCanvas.addEventListener("mousedown", (e) => this.OnMouseDown(e));
        this.spriteCanvas.addEventListener("mouseup", (e) => this.OnMouseUp(e));
        this.spriteCanvas.addEventListener("mousemove", (e) => this.OnMouseMove(e));
        this.spriteCanvas.addEventListener("mouseleave", (e) => this.OnMouseLeave(e));
    }

    async Init(color: Color, time: number, increment: number, squares: Piece[]) {
        if (color === "none") {
            throw new Error("Unexpected color in board init");
        }

        this.isWhite = color === "white";
        this.whiteTime = time;
        this.blackTime = time;
        this.whiteTurn = true;
        this.squares = squares;

        this.DrawBoard();

        await this.LoadSprites();

        if (!this.isWhite) {
            // Flip array
            const newSquares: Piece[] = new Array(64);
            for (let y = 0; y < BOARD_SIZE; y++) {
                const sourceY = BOARD_SIZE - y - 1;

                for (let x = 0; x < BOARD_SIZE; x++) {
                    newSquares[y * 8 + x] = this.squares[sourceY * 8 + x]!;
                }
            }

            this.squares = newSquares;
        }

        this.DrawPieces();
    }

    async LoadSprites() {
        function LoadSprite(url: string): Promise<HTMLImageElement> {
            return new Promise((res, rej) => {
                const svg: HTMLImageElement = new Image();
                svg.onload = () => res(svg);
                svg.onerror = () => rej(new Error(`Failed to load svg: ${url}`));
                svg.crossOrigin = "anonymous";
                svg.src = url;
            });
        }

        // NOTE: Throws an error on failure - Use some fallbacks maybe?
        this.sprites.set("p", await LoadSprite("./assets/sprites/b_pawn_svg_NoShadow.svg"));
        this.sprites.set("n", await LoadSprite("./assets/sprites/b_knight_svg_NoShadow.svg"));
        this.sprites.set("b", await LoadSprite("./assets/sprites/b_bishop_svg_NoShadow.svg"));
        this.sprites.set("r", await LoadSprite("./assets/sprites/b_rook_svg_NoShadow.svg"));
        this.sprites.set("q", await LoadSprite("./assets/sprites/b_queen_svg_NoShadow.svg"));
        this.sprites.set("k", await LoadSprite("./assets/sprites/b_king_svg_NoShadow.svg"));

        console.log("Sprites loaded successfully");
        this.spritesLoaded = true;
    }

    DrawSpriteAt(piece: Piece, x: OctalDigit, y: OctalDigit) {
        if (!this.spritesLoaded) {
            console.warn("Requested sprite draw but not finished loading");
            return;
        }

        const sprite = this.sprites.get(piece.toLowerCase() as Piece)!;
        const squareSize = this.spriteCanvas.width / BOARD_SIZE;

        // The rook sprites are a little larger than I would've liked.
        const pieceScale = piece === "r" || piece === "R" ? ROOK_SCALE : PIECE_SCALE;
        const pieceSize = squareSize * pieceScale;

        const new_x = x - pieceSize / 2;
        const new_y = y - pieceSize / 2;

        // We only load black sprites and then invert colors for white to avoid loading extra
        if (piece !== piece.toLowerCase() || piece === piece.toUpperCase()) {
            this.spriteCtx.filter = "invert(1)";
        }

        this.spriteCtx.drawImage(sprite, new_x, new_y, pieceSize, pieceSize);
        this.spriteCtx.filter = "none";
    }

    OnMouseDown(e: MouseEvent) {
        const targetSquare = new Square(
            Math.floor((e.offsetX / this.boardCanvas.clientWidth) * BOARD_SIZE) as OctalDigit,
            Math.floor((e.offsetY / this.boardCanvas.clientHeight) * BOARD_SIZE) as OctalDigit,
        );
        const targetPiece = this.squares[targetSquare.AsIndex()]!;
        if (targetPiece !== " " && IsWhite(targetPiece) === this.whiteTurn) {
            this.selectedSquare = targetSquare;
            this.draggingPiece = true;
            this.hideSprite = targetSquare.AsIndex();
            this.DrawBoard(movegen.GetMoves(this, this.selectedSquare.AsIndex()));
        } else if (
            this.selectedSquare !== null &&
            !this.draggingPiece &&
            (targetPiece === " " || IsWhite(targetPiece) !== this.whiteTurn)
        ) {
            if (
                movegen
                    .GetMoves(this, this.selectedSquare.AsIndex())
                    .includes(targetSquare.AsIndex())
            ) {
                this.MakeMove(this.selectedSquare, targetSquare);
                this.selectedSquare = null;
                this.DrawBoard();
                this.DrawPieces();
            } else {
                this.selectedSquare = null;
                this.DrawBoard();
                this.DrawPieces();
            }
        }
    }

    OnMouseUp(e: MouseEvent) {
        if (this.draggingPiece) {
            if (this.selectedSquare === null) {
                throw new Error("Expected selected square to be non-null during mouse up");
            }

            const targetSquare = new Square(
                Math.floor((e.offsetX / this.boardCanvas.clientWidth) * BOARD_SIZE) as OctalDigit,
                Math.floor((e.offsetY / this.boardCanvas.clientHeight) * BOARD_SIZE) as OctalDigit,
            );

            if (
                movegen
                    .GetMoves(this, this.selectedSquare.AsIndex())
                    .includes(targetSquare.AsIndex())
            ) {
                this.MakeMove(this.selectedSquare, targetSquare);
                this.selectedSquare = null;
                this.DrawBoard();
            }
        }
        this.hideSprite = -1;
        this.draggingPiece = false;
        this.DrawPieces();
    }

    OnMouseLeave(_e: MouseEvent) {
        this.draggingPiece = false;
        this.selectedSquare = null;
        this.hideSprite = -1;
        this.DrawPieces();
        this.DrawBoard();
    }

    OnMouseMove(e: MouseEvent) {
        if (this.draggingPiece) {
            if (this.selectedSquare === null) {
                throw new Error("Expected selected square to be non-null during mouse move");
            }

            this.DrawPieces();
            this.DrawSpriteAt(
                this.squares[this.selectedSquare.AsIndex()]!,
                e.offsetX as OctalDigit,
                e.offsetY as OctalDigit,
            );
        }
    }

    MakeMove(from: Square, to: Square) {
        const fromIndex: number = from.AsIndex();
        const toIndex: number = to.AsIndex();
        const piece: Piece = this.squares[fromIndex]!;

        // En Passant
        if (piece === "p" || piece === "P") {
            if (toIndex === this.enPassantSquare) {
                if (piece === "p" && this.isWhite) {
                    this.squares[this.enPassantSquare - BOARD_SIZE] = " ";
                } else {
                    this.squares[this.enPassantSquare + BOARD_SIZE] = " ";
                }
            }

            if (Math.abs(fromIndex - toIndex) === 2 * BOARD_SIZE) {
                this.enPassantSquare = (fromIndex + toIndex) / 2;
            } else {
                this.enPassantSquare = -1;
            }
        } else {
            this.enPassantSquare = -1;
        }

        // Board
        this.squares[toIndex] = piece;
        this.squares[fromIndex] = " ";

        // Turn
        this.whiteTurn = !this.whiteTurn;
    }

    DrawBoard(highlight: number[] = []) {
        this.boardCanvas.width = this.boardCanvas.clientWidth;
        this.boardCanvas.height = this.boardCanvas.clientHeight;

        const squareSize: number = this.boardCanvas.width / BOARD_SIZE;

        for (let r: OctalDigit = 0; r < BOARD_SIZE; r++) {
            for (let c: OctalDigit = 0; c < BOARD_SIZE; c++) {
                const isLightSquare: boolean = (r + c) % 2 === 0;

                const x: number = c * squareSize;
                const y: number = r * squareSize;

                var lightSquareCol: string = LIGHT_SQUARE_HEX;
                var darkSquareCol: string = DARK_SQUARE_HEX;
                if (highlight.includes(r * BOARD_SIZE + c)) {
                    lightSquareCol = LIGHT_SQUARE_HIGHLIGHT_HEX;
                    darkSquareCol = DARK_SQUARE_HIGHLIGHT_HEX;
                } else if (
                    this.selectedSquare !== null &&
                    this.selectedSquare.Equals(new Square(c as OctalDigit, r as OctalDigit))
                ) {
                    lightSquareCol = LIGHT_SQUARE_SELECTED_HEX;
                    darkSquareCol = DARK_SQUARE_SELECTED_HEX;
                }

                // Background color
                const width: number =
                    c === BOARD_SIZE - 1 ? this.boardCanvas.width - x : squareSize;
                const height: number =
                    r === BOARD_SIZE - 1 ? this.boardCanvas.height - y : squareSize;

                this.boardCtx.fillStyle = isLightSquare ? lightSquareCol : darkSquareCol;
                this.boardCtx.fillRect(x, y, width, height);

                // Letter
                const label: string = this.isWhite
                    ? String.fromCharCode("A".charCodeAt(0) + c) + (BOARD_SIZE - r)
                    : String.fromCharCode("H".charCodeAt(0) - c) + (r + 1);

                const fontSize: number = Math.floor(squareSize * 0.15);

                this.boardCtx.font = `bold ${fontSize}px arial`;
                this.boardCtx.fillStyle = isLightSquare ? darkSquareCol : lightSquareCol; // Invert
                this.boardCtx.fillText(label, x + 3, y + fontSize);
            }
        }
    }

    DrawPieces() {
        // Don't draw if not done loading yet
        if (!this.spritesLoaded) {
            console.warn("Requested sprite draw but not finished loading");
            return;
        }

        this.spriteCanvas.width = this.spriteCanvas.clientWidth;
        this.spriteCanvas.height = this.spriteCanvas.clientHeight;

        this.spriteCtx.clearRect(0, 0, this.spriteCanvas.width, this.spriteCanvas.height);

        const squareSize: number = this.spriteCanvas.width / BOARD_SIZE;

        for (let i: number = 0; i < BOARD_SIZE * BOARD_SIZE; i++) {
            const piece: Piece = this.squares[i]!;
            const isRook: boolean = piece === "r" || piece === "R";
            if (piece !== " " && i !== this.hideSprite) {
                const sprite: HTMLImageElement = this.sprites.get(piece.toLowerCase() as Piece)!;

                const r: OctalDigit = Math.floor(i / BOARD_SIZE) as OctalDigit;
                const c: OctalDigit = (i % BOARD_SIZE) as OctalDigit;

                // The rook sprites are a little larger than I would've liked.
                const pieceScale: number = isRook ? ROOK_SCALE : PIECE_SCALE;
                const pieceSize: number = squareSize * pieceScale;

                const x: number = squareSize * c + (squareSize - pieceSize) / 2;
                var y: number = squareSize * r + (squareSize - pieceSize) / 2;

                // Shift baseline of rooks down slightly to account for scale.
                if (isRook) {
                    y += squareSize * 0.02;
                }

                // We only load black sprites and then invert colors for white to avoid loading extra
                if (IsWhite(piece)) {
                    this.spriteCtx.filter = "invert(1)";
                }

                this.spriteCtx.drawImage(sprite, x, y, pieceSize, pieceSize);
                this.spriteCtx.filter = "none";
            }
        }
    }
}
