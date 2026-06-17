# Chess Engine

WIP
A chess engine written in C++ with a python api.

## Build The Shared Library

```
mkdir build && cd build
cmake ..
make
```

## Use in Python 3

- Add the `libchess.so` or `libchess.pyd` file to your Python 3 path.
- Import the library with `import libchess`
- Call functions using `libchess.some_function_name(arguments)`
