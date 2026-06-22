#pragma once

#include <cstdint>

uint8_t ToIndex(uint8_t x, uint8_t y);
uint8_t Difference(uint8_t a, uint8_t b);

struct EngineState {
    typedef enum {
        OK,
        ERROR,
        ERROR_MALFORMED_FEN_STRING,
        ERROR_BAD_FEN_POSITIONS,
        ERROR_BAD_FEN_ACTIVE_COLOR,
        ERROR_BAD_FEN_CASTLING_RIGHTS,
        ERROR_BAD_FEN_EN_PASSANT_SQUARE,
        ERROR_BAD_FEN_HALF_MOVE_COUNTER,
        ERROR_BAD_FEN_FULL_MOVE_COUNTER
    } Value;
};

struct Color {
    typedef enum {
        WHITE,
        BLACK,
        MAX_ENUM
    } Value;

    static Value Invalid() { return Value::MAX_ENUM; }
    static bool IsValid(Value value) { return value < Value::MAX_ENUM; }
    static Value Opposite(Value value) 
    { 
        switch (value) {
            case Value::WHITE: return Value::BLACK;
            case Value::BLACK: return Value::WHITE;
            case Value::MAX_ENUM: return Invalid();
        }
    }
};

struct Piece {
    typedef enum {
        PAWN,
        KNIGHT,
        BISHOP,
        ROOK,
        QUEEN,
        KING,
        MAX_ENUM
    } Value;

    static Value Invalid() { return Value::MAX_ENUM; }
    static bool IsValid(Value value) { return value < Value::MAX_ENUM; }
    static const char *Show(Value value)
    {
        switch (value) {
            case PAWN: return "Pawn";
            case KNIGHT: return "Knight";
            case BISHOP: return "Bishop";
            case ROOK: return "Rook";
            case QUEEN: return "Queen";
            case KING: return "King";
            default: return "Invalid";
        }
    }
};

struct CastlingRights {
    bool kingside[2]{true, true};
    bool queenside[2]{true, true};
};

struct FenView {
    uint64_t start;
    uint64_t end;
};
