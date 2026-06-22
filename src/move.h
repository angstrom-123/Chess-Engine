#pragma once

#include "core.h"

union LongAlgebraicMove {
    struct {
        char from[2];
        char to[2];
        char promote;
    };
    char chars[5];

    static LongAlgebraicMove Invalid() { return LongAlgebraicMove { '\0', '\0', '\0', '\0', '\0' }; }
    static bool IsValid(LongAlgebraicMove move) { return move.chars[0] != '\0'; }
};

struct Move {
    uint8_t from{UINT8_MAX};
    uint8_t to{UINT8_MAX};
    Piece::Value piece{Piece::Invalid()};
    Piece::Value promote{Piece::Invalid()};

    static Move Invalid() { return Move(); }
    LongAlgebraicMove ToLAN();
};

