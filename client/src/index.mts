import { Board, getTimeControl, type TimeControl } from "./board.mjs";

interface EngineListResponse {
    engines: string[];
}

interface EngineMoveResponse {
    move_lan: string;
}

interface APICallInfo {
    endpoint: string;
    method: string;
    body?: Object;
}

async function main() {
    const board = new Board();

    // prettier-ignore
    await board.init([
        "r", "n", "b", "q", "k", "b", "n", "r",
        "p", "p", "p", "p", "p", "p", "p", "p",
        " ", " ", " ", " ", " ", " ", " ", " ",
        " ", " ", " ", " ", " ", " ", " ", " ",
        " ", " ", " ", " ", " ", " ", " ", " ",
        " ", " ", " ", " ", " ", " ", " ", " ",
        "P", "P", "P", "P", "P", "P", "P", "P",
        "R", "N", "B", "Q", "K", "B", "N", "R",
    ]);

    // Add playable engines to UI
    const { engines } = (await retryApiCall({
        endpoint: "/engine-list/",
        method: "GET",
    })) as EngineListResponse;

    for (const engine of engines) {
        for (const selectElement of [
            document.getElementById("white-player")!,
            document.getElementById("black-player")!,
        ]) {
            const option: HTMLOptionElement = document.createElement("option") as HTMLOptionElement;
            option.value = engine;
            option.innerText = engine;

            selectElement.appendChild(option);
        }
    }

    // Hide loading menu now that initialization is complete
    const loadingMenu: HTMLDivElement = document.getElementById("loading-menu")! as HTMLDivElement;
    loadingMenu.style.display = "none";

    // Initialize game setup form
    const form: HTMLFormElement = document.getElementById("setup-game-form")! as HTMLFormElement;
    const menu: HTMLDivElement = document.getElementById("setup-game-menu")! as HTMLDivElement;
    form.addEventListener("submit", async (e) => {
        e.preventDefault();
        menu.style.display = "none";

        // Read Form
        const formElement: HTMLFormElement = e.target! as HTMLFormElement;
        const formData: FormData = new FormData(formElement);

        const whitePlayer: string = formData.get("white-player")! as string;
        const blackPlayer: string = formData.get("black-player")! as string;
        const timeControl: TimeControl = formData.get("time-control")! as TimeControl;

        const friendlyLabel: HTMLParagraphElement = document.getElementById(
            "friendly-label",
        ) as HTMLParagraphElement;
        const opponentLabel: HTMLParagraphElement = document.getElementById(
            "opponent-label",
        ) as HTMLParagraphElement;

        console.log(whitePlayer, blackPlayer);

        var flipBoard: boolean = false;
        if (whitePlayer === "Local" || blackPlayer !== "Local") {
            friendlyLabel.innerText = whitePlayer;
            opponentLabel.innerText = blackPlayer;
        } else {
            friendlyLabel.innerText = blackPlayer;
            opponentLabel.innerText = whitePlayer;
            flipBoard = true;
        }

        const [seconds, increment] = getTimeControl(timeControl);
        await retryApiCall({
            endpoint: "/game-start/",
            method: "POST",
            body: {
                white_player: whitePlayer,
                black_player: blackPlayer,
                time_control: {
                    seconds: seconds,
                    increment: increment,
                },
            },
        });

        await board.start({
            whitePlayer: whitePlayer,
            blackPlayer: blackPlayer,
            timeControl: timeControl,
            flipBoard: flipBoard,
            getMoveApiCall: getEngineMove,
            makeMoveApiCall: makeEngineMove,
        });
    });
}

async function makeEngineMove(moveLan: string): Promise<void> {
    await retryApiCall({
        endpoint: "/make-move/",
        method: "POST",
        body: {
            move_lan: moveLan,
        },
    });
}

async function getEngineMove(timeLeftMs: number): Promise<string> {
    const { move_lan } = (await retryApiCall({
        endpoint: "/best-move/",
        method: "POST",
        body: {
            ms_left: timeLeftMs,
        },
    })) as EngineMoveResponse;
    return move_lan;
}

async function retryApiCall(callInfo: APICallInfo, retries: number = 2): Promise<Object> {
    for (let i = 0; i < retries; i++) {
        const res: Object | undefined = await apiCall(callInfo);
        if (res !== undefined) return res;
    }
    throw new Error(`API call to ${callInfo.endpoint} failed after ${retries} retries`);
}

async function apiCall({ endpoint, method, body }: APICallInfo): Promise<Object | undefined> {
    const request: RequestInit = {
        method: method,
        headers: {
            "Content-Type": "application/json",
            Accept: "application/json",
        },
    };
    // Only add a body field if one is supplied
    if (body !== undefined) request.body = JSON.stringify(body);

    const url = "http://localhost:8000" + endpoint;
    return await fetch(url, request).then(async (res: Response) => {
        if (!res.ok) {
            console.error(`API Call failed to '${endpoint}'`);
            return undefined;
        }

        return await res.json();
    });
}

main();
