import { Board, getTimeControl, type Color, type TimeControl } from "./board.mjs";

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

    // Hide loading menu now that initialization is complete
    const loadingMenu: HTMLElement = document.getElementById("loading-menu")!;
    loadingMenu.style.display = "none";

    const form: HTMLElement = document.getElementById("setup-game-form")!;
    const menu: HTMLElement = document.getElementById("setup-game-menu")!;
    form.addEventListener("submit", async (e) => {
        e.preventDefault();
        menu.style.display = "none";

        const formElement: HTMLFormElement = e.target! as HTMLFormElement;
        const formData: FormData = new FormData(formElement);

        const playAs: Color = formData.get("play-as")! as Color;
        const playAgainst: string = formData.get("play-against")! as string;
        const timeControl: TimeControl = formData.get("time-control")! as TimeControl;

        await apiCall("/game-start/", "POST", {
            play_as: playAs,
            play_against: playAgainst,
            time_control: {
                time: getTimeControl(timeControl)[0],
                increment: getTimeControl(timeControl)[1],
            },
        });

        board.start(playAs, timeControl);
    });
}

async function apiCall(endpoint: string, method: string, body: Object = {}): Promise<Object> {
    const url = "http://localhost:8000" + endpoint;
    console.log("String version\n" + JSON.stringify(body));
    return await fetch(url, {
        method: method,
        headers: {
            Accept: "application/json",
        },
        body: JSON.stringify(body),
    }).then(async (res: Response) => {
        if (!res.ok) {
            console.error(`API Call failed to '${endpoint}'`);
            return {};
        }

        return await res.json();
    });
}

main();
