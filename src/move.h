#pragma once

#include "bitboard.h"
#include "core.h"

union LongAlgebraicMove {
    struct {
        char from[2];
        char to[2];
        char promote;
    };
    char chars[5];

    static LongAlgebraicMove Invalid() { return LongAlgebraicMove { .chars = { '\0', '\0', '\0', '\0', '\0' } }; }
    static bool IsValid(LongAlgebraicMove move) { return move.chars[0] != '\0'; }
    static LongAlgebraicMove FromChars(char *chars);
};

struct Move {
    uint8_t from{UINT8_MAX};
    uint8_t to{UINT8_MAX};
    uint8_t piece{static_cast<uint8_t>(Piece::Invalid())};
    uint8_t promote{static_cast<uint8_t>(Piece::Invalid())};

    static bool IsValid(Move move) { return Piece::IsValid(move.GetPiece()); }
    static Move Invalid() { return Move(); }
    Piece::Value GetPiece() const { return static_cast<Piece::Value>(piece); }
    Piece::Value GetPromote() const { return static_cast<Piece::Value>(promote); }
    LongAlgebraicMove ToLAN();
    static Move FromLAN(LongAlgebraicMove lan, const BitboardSet& piecePositions);
};

