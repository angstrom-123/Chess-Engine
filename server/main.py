import mimetypes
from fastapi import FastAPI, Request, status
from pydantic import BaseModel

mimetypes.add_type("text/javascript", ".mjs")

class TimeControlInfo(BaseModel):
    time: int 
    increment: int

class GameStartInfo(BaseModel):
    play_as: str
    play_against: str 
    time_control: TimeControlInfo

app = FastAPI()

# TODO: In the backend add an api for the engine to get moves and make moves and stuff 
#       Then, add a way to implement this in another file and just register it with a name 
#       Then, add a way for the frontend to get this list of registered engines and select one 
#       Finally, write in the docs how to set up a new engine
#       Eventually, add a way for the frontend to select 2 engines to play against each-other

@app.post("/game-start/", status_code=status.HTTP_200_OK)
async def game_start(request: Request) -> dict[None, None]:
    info: GameStartInfo = GameStartInfo.model_validate_json(await request.body())
    print(info)
    return {}

app.frontend("/", directory="client", fallback="index.html")
