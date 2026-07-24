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

export const TimeControls = ["0:30", "1", "5", "10", "1+1", "5+5", "10+10"] as const;
export type TimeControl = (typeof TimeControls)[number];

export function getTimeControl(timeControl: TimeControl): [number, number] {
    switch (timeControl) {
        case "0:30":
            return [30, 0];
        case "1":
            return [60, 0];
        case "5":
            return [300, 0];
        case "5":
            return [300, 0];
        case "1+1":
            return [60, 1];
        case "5+5":
            return [300, 5];
        case "10+10":
            return [600, 10];
        default:
            throw new Error("Bad time control");
    }
}

export class Square {
    public x: OctalDigit;
    public y: OctalDigit;

    constructor(x: OctalDigit, y: OctalDigit) {
        this.x = x;
        this.y = y;
    }

    static fromIndex(index: number): Square {
        return new Square((index % 8) as OctalDigit, Math.floor(index / 8) as OctalDigit);
    }

    public asIndex(): number {
        return this.y * BOARD_SIZE + this.x;
    }

    public equals(square: Square): boolean {
        return square.x === this.x && square.y === this.y;
    }
}

export class MoveData {
    public from: number;
    public to: number;
    public piece: Piece;
    public capture: Piece;
    public enPassantSquare: number;
    public whiteCanCastleLong: boolean;
    public whiteCanCastleShort: boolean;
    public blackCanCastleLong: boolean;
    public blackCanCastleShort: boolean;
    public whiteTurn: boolean;
    public isCastles: boolean;
    public isPromotion: boolean;

    private boardFlipped: boolean;
    private promotedTo: Piece = " ";

    public toLan(): string {
        var fromCol: number = this.from % BOARD_SIZE;
        var fromRow: number = 7 - Math.floor(this.from / BOARD_SIZE);
        var toCol: number = this.to % BOARD_SIZE;
        var toRow: number = 7 - Math.floor(this.to / BOARD_SIZE);

        if (this.boardFlipped) {
            fromCol = 7 - fromCol;
            fromRow = 7 - fromRow;
            toCol = 7 - toCol;
            toRow = 7 - toRow;
        }

        const fromColChar: string = String.fromCharCode("a".charCodeAt(0) + fromCol);
        const fromRowChar: string = String.fromCharCode("1".charCodeAt(0) + fromRow);
        const toColChar: string = String.fromCharCode("a".charCodeAt(0) + toCol);
        const toRowChar: string = String.fromCharCode("1".charCodeAt(0) + toRow);

        var lan: string = fromColChar + fromRowChar + toColChar + toRowChar;
        if (this.isPromotion) lan += this.promotedTo;

        return lan;
    }

    public setPromotion(piece: Piece) {
        this.promotedTo = piece;
    }

    constructor(from: number, to: number, board: Board) {
        this.from = from;
        this.to = to;
        this.piece = board.squares[from]! as Piece;
        this.capture = board.squares[to]! as Piece;
        this.enPassantSquare = board.enPassantSquare;
        this.whiteCanCastleLong = board.whiteCanCastleLong;
        this.whiteCanCastleShort = board.whiteCanCastleShort;
        this.blackCanCastleLong = board.blackCanCastleLong;
        this.blackCanCastleShort = board.blackCanCastleShort;
        this.whiteTurn = board.whiteTurn;
        this.isCastles = (this.piece === "k" || this.piece === "K") && Math.abs(from - to) === 2;
        this.isPromotion = (this.piece === "p" || this.piece === "P") && (to < 8 || to > 55);

        this.boardFlipped = board.boardFlipped;
    }
}

export function sameColor(piece0: Piece, piece1: Piece) {
    return isWhite(piece0) === isWhite(piece1);
}

export function isWhite(piece: Piece) {
    return piece.charCodeAt(0) < 91;
}

export interface BoardStartInfo {
    whitePlayer: string;
    blackPlayer: string;
    timeControl: TimeControl;
    flipBoard: boolean;
    getMoveApiCall: (timeLeftMs: number) => Promise<string>;
    makeMoveApiCall: (moveLan: string) => Promise<void>;
}

export class Board {
    public squares: Piece[];
    public attacks: boolean[][];
    public boardFlipped: boolean = false;
    public enPassantSquare: number = -1;
    public whiteCanCastleLong: boolean = true;
    public whiteCanCastleShort: boolean = true;
    public blackCanCastleLong: boolean = true;
    public blackCanCastleShort: boolean = true;
    public whiteTurn: boolean = true;
    public whitePlayer: string = "";
    public blackPlayer: string = "";

    private started: boolean = false;
    private waitForStart: boolean = false;
    private selectedSquare: Square | null = null;
    private draggingPiece: boolean = false;
    private hideSprite: number = -1;
    private sprites: Map<Piece, HTMLImageElement>;
    private spritesLoaded: boolean = false;
    private promotion: MoveData | null = null;
    private promotionMenu: HTMLDivElement;
    private promoteKnightButton: HTMLButtonElement;
    private promoteBishopButton: HTMLButtonElement;
    private promoteRookButton: HTMLButtonElement;
    private promoteQueenButton: HTMLButtonElement;
    private boardCanvas: HTMLCanvasElement;
    private spriteCanvas: HTMLCanvasElement;
    private boardCtx: CanvasRenderingContext2D;
    private spriteCtx: CanvasRenderingContext2D;
    private getMoveApiCall: ((timeLeftMs: number) => Promise<string>) | undefined = undefined;
    private makeMoveApiCall: ((moveLan: string) => Promise<void>) | undefined = undefined;

    public friendlyTimer: CountdownTimer | null = null;
    public opponentTimer: CountdownTimer | null = null;

    constructor() {
        this.squares = new Array<Piece>(64) as Piece[];
        this.attacks = [new Array<boolean>(64), new Array<boolean>(64)];
        this.sprites = new Map<Piece, HTMLImageElement>();

        this.promotionMenu = document.getElementById("promotion-menu") as HTMLDivElement;
        this.promoteKnightButton = document.getElementById("promote-knight") as HTMLButtonElement;
        this.promoteBishopButton = document.getElementById("promote-bishop") as HTMLButtonElement;
        this.promoteRookButton = document.getElementById("promote-rook") as HTMLButtonElement;
        this.promoteQueenButton = document.getElementById("promote-queen") as HTMLButtonElement;
        this.boardCanvas = document.getElementById("board-canvas")! as HTMLCanvasElement;
        this.spriteCanvas = document.getElementById("sprite-canvas")! as HTMLCanvasElement;

        const boardContext: CanvasRenderingContext2D | null = this.boardCanvas.getContext("2d");
        const spriteContext: CanvasRenderingContext2D | null = this.spriteCanvas.getContext("2d");
        if (boardContext === null || spriteContext === null) {
            throw new Error(
                "Could not obtain rendering context for board canvas nor sprite canvas",
            );
        }
        this.boardCtx = boardContext;
        this.spriteCtx = spriteContext;

        this.spriteCanvas.addEventListener("mousedown", (e) => this.onMouseDown(e));
        this.spriteCanvas.addEventListener("mouseup", (e) => this.onMouseUp(e));
        this.spriteCanvas.addEventListener("mousemove", (e) => this.onMouseMove(e));
        this.spriteCanvas.addEventListener("mouseleave", (e) => this.onMouseLeave(e));
        this.promoteKnightButton.addEventListener("click", async (e) => this.onPromote(e, "n"));
        this.promoteBishopButton.addEventListener("click", async (e) => this.onPromote(e, "b"));
        this.promoteRookButton.addEventListener("click", async (e) => this.onPromote(e, "r"));
        this.promoteQueenButton.addEventListener("click", async (e) => this.onPromote(e, "q"));
    }

    public async init(squares: Piece[]) {
        this.squares = squares;
        this.drawBoard();
        await this.loadSprites();
    }

    public async start({
        whitePlayer,
        blackPlayer,
        timeControl,
        flipBoard,
        getMoveApiCall,
        makeMoveApiCall,
    }: BoardStartInfo) {
        this.whitePlayer = whitePlayer;
        this.blackPlayer = blackPlayer;
        this.boardFlipped = flipBoard;
        this.getMoveApiCall = getMoveApiCall;
        this.makeMoveApiCall = makeMoveApiCall;

        const [time, increment] = getTimeControl(timeControl);

        this.friendlyTimer = new CountdownTimer(
            time * 1000,
            increment * 1000,
            10,
            document.getElementById("friendly-timer")!,
        );
        this.opponentTimer = new CountdownTimer(
            time * 1000,
            increment * 1000,
            10,
            document.getElementById("opponent-timer")!,
        );
        this.whiteTurn = true;

        if (this.boardFlipped) {
            // Flip array
            const newSquares: Piece[] = new Array(64);
            for (let y = 0; y < BOARD_SIZE; y++) {
                const sourceY = BOARD_SIZE - y - 1;

                for (let x = 0; x < BOARD_SIZE; x++) {
                    newSquares[y * 8 + (7 - x)] = this.squares[sourceY * 8 + x]!;
                }
            }

            this.squares = newSquares;
        }

        this.drawBoard(); // Need to redraw in case playing as black (needs flip)
        this.drawPieces();

        if (this.whitePlayer === "Local") {
            this.waitForStart = true;
        } else {
            this.startGame();
            const move: string = await this.getMoveApiCall(
                !this.boardFlipped ? this.friendlyTimer.getMs() : this.opponentTimer.getMs(),
            );
            await this.applyMoveLan(move);
        }
    }

    public getRemainingMs(): number {
        if (this.friendlyTimer === null || this.opponentTimer === null)
            throw new Error("Getting remaining time for uninitialized timers");

        if ((this.whiteTurn && !this.boardFlipped) || (!this.whiteTurn && this.boardFlipped))
            return this.friendlyTimer.getMs();
        return this.opponentTimer.getMs();
    }

    public async applyMoveLan(lan: string) {
        if (this.makeMoveApiCall === undefined)
            throw new Error("Make Move API Call must be defined");

        await this.makeMoveApiCall(lan);
        const [from, to, promote] = this.parseLAN(lan);
        const moveData: MoveData = this.makeMove(from, to);
        switch (promote) {
            case "n":
                this.squares[to.asIndex()] = moveData.whiteTurn ? "N" : "n";
            case "b":
                this.squares[to.asIndex()] = moveData.whiteTurn ? "B" : "b";
            case "r":
                this.squares[to.asIndex()] = moveData.whiteTurn ? "R" : "r";
            case "q":
                this.squares[to.asIndex()] = moveData.whiteTurn ? "Q" : "q";
        }
        this.updateAttacks();
        await this.updateGame();
    }

    public makeMove(from: Square, to: Square): MoveData {
        const fromIndex: number = from.asIndex();
        const toIndex: number = to.asIndex();
        const piece: Piece = this.squares[fromIndex]!;

        const moveData: MoveData = new MoveData(fromIndex, toIndex, this);

        // En Passant
        if (piece === "p" || piece === "P") {
            if (toIndex === this.enPassantSquare) {
                if (
                    (!isWhite(piece) && !this.boardFlipped) ||
                    (isWhite(piece) && this.boardFlipped)
                ) {
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

        // Castling
        if ((piece === "k" || piece === "K") && Math.abs(fromIndex - toIndex) === 2) {
            if (fromIndex > toIndex) {
                // Castle long
                const rookIndex = fromIndex - (this.boardFlipped ? 3 : 4);
                this.squares[toIndex + 1] = this.squares[rookIndex]!;
                this.squares[rookIndex] = " ";
            } else {
                // Castle short
                const rookIndex = fromIndex + (this.boardFlipped ? 4 : 3);
                this.squares[toIndex - 1] = this.squares[rookIndex]!;
                this.squares[rookIndex] = " ";
            }
        }

        // Castling rights
        {
            // King moved
            if (piece === "k") {
                this.blackCanCastleLong = false;
                this.blackCanCastleShort = false;
            } else if (piece === "K") {
                this.whiteCanCastleLong = false;
                this.whiteCanCastleShort = false;
            }

            // Rook moved
            if (piece === "r") {
                if (fromIndex === 0 || fromIndex === 56) this.blackCanCastleLong = false;
                else if (fromIndex === 7 || fromIndex === 63) this.blackCanCastleShort = false;
            } else if (piece === "R") {
                if (fromIndex === 0 || fromIndex === 56) this.whiteCanCastleLong = false;
                else if (fromIndex === 7 || fromIndex === 63) this.whiteCanCastleShort = false;
            }

            // Rook captured
            if ((!this.boardFlipped && toIndex === 56) || (this.boardFlipped && toIndex === 0))
                this.whiteCanCastleLong = false;
            if ((!this.boardFlipped && toIndex === 63) || (this.boardFlipped && toIndex === 7))
                this.whiteCanCastleShort = false;
            if ((!this.boardFlipped && toIndex === 0) || (this.boardFlipped && toIndex === 56))
                this.blackCanCastleLong = false;
            if ((!this.boardFlipped && toIndex === 7) || (this.boardFlipped && toIndex === 63))
                this.blackCanCastleShort = false;
        }

        // Board
        this.squares[toIndex] = piece;
        this.squares[fromIndex] = " ";

        // Turn
        this.whiteTurn = !this.whiteTurn;

        return moveData;
    }

    public unmakeMove(moveData: MoveData) {
        // En Passant
        const piece: Piece = moveData.piece;
        const capture: Piece = moveData.capture;
        const fromIndex: number = moveData.from;
        const toIndex: number = moveData.to;

        if ((piece === "p" || piece === "P") && toIndex === moveData.enPassantSquare) {
            if ((!isWhite(piece) && !this.boardFlipped) || (isWhite(piece) && this.boardFlipped)) {
                this.squares[moveData.enPassantSquare - BOARD_SIZE] = isWhite(moveData.piece)
                    ? "p"
                    : "P";
            } else {
                this.squares[moveData.enPassantSquare + BOARD_SIZE] = isWhite(moveData.piece)
                    ? "p"
                    : "P";
            }
        }
        this.enPassantSquare = moveData.enPassantSquare;

        // Castling
        if ((piece === "k" || piece === "K") && Math.abs(fromIndex - toIndex) === 2) {
            if (fromIndex > toIndex) {
                this.squares[fromIndex - (this.boardFlipped ? 3 : 4)] = isWhite(piece) ? "R" : "r";
                this.squares[fromIndex - 1] = " ";
            } else {
                this.squares[fromIndex + (this.boardFlipped ? 4 : 3)] = isWhite(piece) ? "R" : "r";
                this.squares[fromIndex + 1] = " ";
            }
        }
        this.whiteCanCastleLong = moveData.whiteCanCastleLong;
        this.whiteCanCastleShort = moveData.whiteCanCastleShort;
        this.blackCanCastleLong = moveData.blackCanCastleLong;
        this.blackCanCastleShort = moveData.blackCanCastleShort;

        // Board
        this.squares[fromIndex] = piece;
        this.squares[toIndex] = capture;

        // Turn
        this.whiteTurn = moveData.whiteTurn;
    }

    public promote(moveData: MoveData) {
        this.promotion = moveData;
        this.promotionMenu.style.display = "inline";
    }

    public updateAttacks() {
        this.attacks[0] = this.getAllAttacks("white");
        this.attacks[1] = this.getAllAttacks("black");
    }

    public async updateGame() {
        if (this.getMoveApiCall === undefined) throw new Error("Get move API call must be defined");

        if ((this.whiteTurn && !this.boardFlipped) || (!this.whiteTurn && this.boardFlipped)) {
            this.friendlyTimer!.start();
            this.opponentTimer!.stop();
        } else {
            this.friendlyTimer!.stop();
            this.opponentTimer!.start();
        }

        this.drawPieces();

        // Get engine move now if required
        if (this.whiteTurn && this.whitePlayer !== "Local") {
            const moveLan: string = await this.getMoveApiCall(
                !this.boardFlipped ? this.friendlyTimer!.getMs() : this.opponentTimer!.getMs(),
            );
            await this.applyMoveLan(moveLan);
            this.drawPieces();
        } else if (!this.whiteTurn && this.blackPlayer !== "Local") {
            const moveLan: string = await this.getMoveApiCall(
                !this.boardFlipped ? this.opponentTimer!.getMs() : this.friendlyTimer!.getMs(),
            );
            await this.applyMoveLan(moveLan);
            this.drawPieces();
        }
    }

    public getAllAttacks(color: Color) {
        var allAttacks: boolean[] = new Array<boolean>(64);
        for (let i: number = 0; i < BOARD_SIZE * BOARD_SIZE; i++) {
            if (
                this.squares[i] === " " ||
                isWhite(this.squares[i] as Piece) !== (color === "white")
            ) {
                continue;
            }

            const attacks: number[] = movegen.getAttacks(this, i);
            for (const attack of attacks) {
                allAttacks[attack]! = true;
            }
        }
        return allAttacks;
    }

    private startGame() {
        this.started = true;
        this.waitForStart = false;
        if (!this.boardFlipped) {
            this.friendlyTimer!.start();
        } else {
            this.opponentTimer!.start();
        }
    }

    private parseLAN(lan: string): [Square, Square, Piece | undefined] {
        const fromCol: number = lan.charCodeAt(0);
        const fromRow: number = lan.charCodeAt(1);
        const toCol: number = lan.charCodeAt(2);
        const toRow: number = lan.charCodeAt(3);
        const promotion: Piece | undefined =
            lan.length === 5 ? (lan.charAt(4) as Piece) : undefined;

        var fromSquare: Square = new Square(
            (fromCol - "a".charCodeAt(0)) as OctalDigit,
            (7 - (fromRow - "1".charCodeAt(0))) as OctalDigit,
        );
        var toSquare: Square = new Square(
            (toCol - "a".charCodeAt(0)) as OctalDigit,
            (7 - (toRow - "1".charCodeAt(0))) as OctalDigit,
        );

        if (this.boardFlipped) {
            fromSquare.x = (7 - fromSquare.x) as OctalDigit;
            fromSquare.y = (7 - fromSquare.y) as OctalDigit;
            toSquare.x = (7 - toSquare.x) as OctalDigit;
            toSquare.y = (7 - toSquare.y) as OctalDigit;
        }

        return [fromSquare, toSquare, promotion];
    }

    private async loadSprites() {
        async function loadSprite(url: string): Promise<HTMLImageElement> {
            // Inject missing fields for chrome
            const response: Response = await fetch(url);
            const rawSvg: string = await response.text();

            const parser: DOMParser = new DOMParser();
            const doc: Document = parser.parseFromString(rawSvg, "image/svg+xml");
            const svgElement: HTMLElement = doc.documentElement;

            const w: string = svgElement.getAttribute("width")?.replace("px", "") || "100";
            const h: string = svgElement.getAttribute("height")?.replace("px", "") || "100";

            if (!svgElement.getAttribute("viewbox")) {
                svgElement.setAttribute("viewbox", `0 0 ${w} ${h}`);
            }
            svgElement.setAttribute("preserveAspectRatio", "xMidYMid meet");
            svgElement.setAttribute("width", w);
            svgElement.setAttribute("height", h);

            const serialized: string = new XMLSerializer().serializeToString(doc);
            const blob: Blob = new Blob([serialized], { type: "image/svg+xml" });
            const blobUrl: string = URL.createObjectURL(blob);

            // Load
            return new Promise((res, rej) => {
                const svg: HTMLImageElement = new Image();
                svg.onload = () => {
                    // Set these for chrome
                    svg.width = parseInt(w);
                    svg.height = parseInt(h);
                    svg.style.width = "100%";
                    svg.style.height = "auto";
                    svg.style.aspectRatio = `${svg.naturalWidth} / ${svg.naturalHeight}`;

                    // Clean up
                    URL.revokeObjectURL(blobUrl);
                    res(svg);
                };
                svg.onerror = () => rej(new Error(`Failed to load svg: ${url}`));
                svg.crossOrigin = "anonymous";
                svg.src = blobUrl;
            });
        }

        // NOTE: Throws an error on failure - Use some fallbacks maybe?
        this.sprites.set("p", await loadSprite("./assets/sprites/b_pawn_svg_NoShadow.svg"));
        this.sprites.set("n", await loadSprite("./assets/sprites/b_knight_svg_NoShadow.svg"));
        this.sprites.set("b", await loadSprite("./assets/sprites/b_bishop_svg_NoShadow.svg"));
        this.sprites.set("r", await loadSprite("./assets/sprites/b_rook_svg_NoShadow.svg"));
        this.sprites.set("q", await loadSprite("./assets/sprites/b_queen_svg_NoShadow.svg"));
        this.sprites.set("k", await loadSprite("./assets/sprites/b_king_svg_NoShadow.svg"));

        console.log("Sprites loaded successfully");
        this.spritesLoaded = true;
    }

    private drawSpriteAt(piece: Piece, x: OctalDigit, y: OctalDigit) {
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

    private async onPromote(_e: MouseEvent, piece: Piece) {
        const square: number = this.promotion!.to;
        this.squares[this.promotion!.to] = isWhite(this.squares[square]!)
            ? (piece.toUpperCase() as Piece)
            : (piece.toLowerCase() as Piece);

        this.drawPieces();

        this.promotion!.setPromotion(piece);
        await this.makeMoveApiCall!(this.promotion!.toLan());
        this.updateAttacks();

        this.promotionMenu.style.display = "none";
        this.promotion = null;
        await this.updateGame();
    }

    private async onMouseDown(e: MouseEvent) {
        if (this.waitForStart) this.startGame();
        if (!this.started) return;
        if (
            (this.whiteTurn && this.whitePlayer !== "Local") ||
            (!this.whiteTurn && this.blackPlayer !== "Local")
        )
            return;
        if (this.makeMoveApiCall === undefined)
            throw new Error("Make move API call must be defined");

        const targetSquare = new Square(
            Math.floor((e.offsetX / this.boardCanvas.clientWidth) * BOARD_SIZE) as OctalDigit,
            Math.floor((e.offsetY / this.boardCanvas.clientHeight) * BOARD_SIZE) as OctalDigit,
        );
        const targetPiece = this.squares[targetSquare.asIndex()]!;
        if (targetPiece !== " " && isWhite(targetPiece) === this.whiteTurn) {
            this.selectedSquare = targetSquare;
            this.draggingPiece = true;
            this.hideSprite = targetSquare.asIndex();
            this.drawBoard(movegen.getMoves(this, this.selectedSquare.asIndex()));
        } else if (
            this.selectedSquare !== null &&
            !this.draggingPiece &&
            (targetPiece === " " || isWhite(targetPiece) !== this.whiteTurn)
        ) {
            if (
                movegen
                    .getMoves(this, this.selectedSquare.asIndex())
                    .includes(targetSquare.asIndex())
            ) {
                const moveData: MoveData = this.makeMove(this.selectedSquare, targetSquare);
                this.selectedSquare = null;
                this.drawBoard();
                this.drawPieces();

                if (moveData.isPromotion) {
                    this.promote(moveData);
                    // API Call and other stuff handled in the callback (onPromote)
                } else {
                    await this.makeMoveApiCall(moveData.toLan());
                    this.updateAttacks();
                    await this.updateGame();
                }
            } else {
                this.selectedSquare = null;
                this.drawBoard();
                this.drawPieces();
            }
        }
    }

    private async onMouseUp(e: MouseEvent) {
        if (!this.started) return;
        if (
            (this.whiteTurn && this.whitePlayer !== "Local") ||
            (!this.whiteTurn && this.blackPlayer !== "Local")
        )
            return;
        if (this.makeMoveApiCall === undefined)
            throw new Error("Make move API call must be defined");

        if (this.draggingPiece) {
            if (this.selectedSquare === null)
                throw new Error("Expected selected square to be non-null during mouse up");

            const targetSquare = new Square(
                Math.floor((e.offsetX / this.boardCanvas.clientWidth) * BOARD_SIZE) as OctalDigit,
                Math.floor((e.offsetY / this.boardCanvas.clientHeight) * BOARD_SIZE) as OctalDigit,
            );

            if (
                movegen
                    .getMoves(this, this.selectedSquare.asIndex())
                    .includes(targetSquare.asIndex())
            ) {
                const moveData: MoveData = this.makeMove(this.selectedSquare, targetSquare);
                if (moveData.isPromotion) this.promote(moveData);
                await this.makeMoveApiCall(moveData.toLan());
                this.updateAttacks();

                this.selectedSquare = null;
                this.drawBoard();

                await this.updateGame();
            }
        }
        this.hideSprite = -1;
        this.draggingPiece = false;
        this.drawPieces();
    }

    private onMouseLeave(_e: MouseEvent) {
        if (!this.started) return;
        if (
            (this.whiteTurn && this.whitePlayer !== "Local") ||
            (!this.whiteTurn && this.blackPlayer !== "Local")
        )
            return;

        this.draggingPiece = false;
        this.selectedSquare = null;
        this.hideSprite = -1;
        this.drawPieces();
        this.drawBoard();
    }

    private onMouseMove(e: MouseEvent) {
        if (!this.started) return;
        if (
            (this.whiteTurn && this.whitePlayer !== "Local") ||
            (!this.whiteTurn && this.blackPlayer !== "Local")
        )
            return;

        if (this.draggingPiece) {
            if (this.selectedSquare === null) {
                throw new Error("Expected selected square to be non-null during mouse move");
            }

            this.drawPieces();
            this.drawSpriteAt(
                this.squares[this.selectedSquare.asIndex()]!,
                e.offsetX as OctalDigit,
                e.offsetY as OctalDigit,
            );
        }
    }

    private drawBoard(highlight: number[] = []) {
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
                    this.selectedSquare.equals(new Square(c as OctalDigit, r as OctalDigit))
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
                const label: string = !this.boardFlipped
                    ? String.fromCharCode("A".charCodeAt(0) + c) + (BOARD_SIZE - r)
                    : String.fromCharCode("H".charCodeAt(0) - c) + (r + 1);

                const fontSize: number = Math.floor(squareSize * 0.15);

                this.boardCtx.font = `bold ${fontSize}px arial`;
                this.boardCtx.fillStyle = isLightSquare ? darkSquareCol : lightSquareCol; // Invert
                this.boardCtx.fillText(label, x + 3, y + fontSize);
            }
        }
    }

    private drawPieces() {
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
                if (isWhite(piece)) {
                    this.spriteCtx.filter = "invert(1)";
                }

                this.spriteCtx.drawImage(sprite, x, y, pieceSize, pieceSize);
                this.spriteCtx.filter = "none";
            }
        }
    }
}

class CountdownTimer {
    private countMs: number;
    private incrementMs: number;
    private display: HTMLElement;

    private isRunning: boolean = false;
    private startTime: number = 0;
    private expectedTime: number = 0;
    private intervalMs: number = 200;
    private timeout: number = 0;

    constructor(fromMs: number, incrementMs: number, updateHz: number, display: HTMLElement) {
        this.countMs = fromMs;
        this.incrementMs = incrementMs;
        this.intervalMs = 1000 / updateHz;
        this.display = display;
        this.display.innerText = this.formatTime();
    }

    public start() {
        if (this.isRunning) {
            throw new Error("Countdown timer is already running");
        }

        this.isRunning = true;
        this.startTime = Date.now();
        this.expectedTime = this.startTime + this.intervalMs;

        this.timeout = setTimeout(() => this.step(), this.intervalMs);
    }

    public stop() {
        if (!this.isRunning) throw new Error("Countdown timer is not running");

        if (this.incrementMs > 0) {
            this.countMs += this.incrementMs;
            this.display.innerText = this.formatTime();
        }

        this.isRunning = false;
        clearTimeout(this.timeout);
    }

    public getMs(): number {
        return this.countMs;
    }

    private step() {
        const drift: number = Date.now() - this.expectedTime;
        if (drift > this.intervalMs) {
            console.warn("Large timer drift detected");
        }

        this.countMs -= this.intervalMs;
        this.display.innerText = this.formatTime();
        this.expectedTime += this.intervalMs;
        this.timeout = setTimeout(() => this.step(), this.intervalMs - drift);
    }

    private formatTime(): string {
        const countSeconds = this.countMs / 1000;

        var lhs: string;
        var rhs: string;

        if (countSeconds <= 60) {
            const seconds: number = Math.floor(countSeconds);
            const millis: number = parseFloat((countSeconds - seconds).toFixed(2)) * 100;

            lhs = seconds.toString();
            rhs = millis.toFixed(0);
            if (rhs.length === 1) rhs += "0";
            else if (rhs.length > 2) rhs = rhs.substring(0, 2);
        } else {
            const minutes: number = Math.floor(countSeconds / 60);
            const seconds: number = Math.floor(countSeconds - minutes * 60);

            lhs = minutes.toString();
            rhs = seconds.toString();
            if (rhs.length === 1) rhs = "0" + rhs;
            else if (rhs.length > 2) rhs = rhs.substring(0, 2);
        }

        return `${lhs}:${rhs}`;
    }
}
