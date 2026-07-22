from server.engine import BaseEngine

from server.jupiter import Jupiter

# Any engines need to be registered here
engines: list[type[BaseEngine]] = [
    Jupiter,
]

