import { BOARD_SIZE, Square, Board } from "./board.mjs";

async function Main() {
    const board = new Board();

    // prettier-ignore
    await board.Init("white", 60, 1, [
        "r", "n", "b", "q", "k", "b", "n", "r",
        "p", "p", "p", "p", "p", "p", "p", "p",
        " ", " ", " ", " ", " ", " ", " ", " ",
        " ", " ", " ", " ", " ", " ", " ", " ",
        " ", " ", " ", " ", " ", " ", " ", " ",
        " ", " ", " ", " ", " ", " ", " ", " ",
        "P", "P", "P", "P", "P", "P", "P", "P",
        "R", "N", "B", "Q", "K", "B", "N", "R",
    ]);

    board.DrawBoard();
    board.DrawPieces();
}

async function APICall(endpoint: string) {
    const url = "http://localhost:8000" + endpoint;
    try {
        const response = await fetch(url, {
            method: "GET",
            headers: {
                Accept: "application/json",
            },
        });
        if (!response.ok) {
            console.error("API Call failed with status: " + response.status);
        }

        const value = await response.json();
        return value;
    } catch (e: any) {
        console.error("API Call failed: " + e.message);
        return {};
    }
}

Main();
