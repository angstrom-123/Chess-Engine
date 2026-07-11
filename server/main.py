import mimetypes
from fastapi import FastAPI

mimetypes.add_type("text/javascript", ".mjs")

app = FastAPI()

# TODO: Add endpoints here

app.frontend("/", directory="client", fallback="index.html")
