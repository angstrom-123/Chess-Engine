#pragma once

#include "bitboard.h"

class BoardState {
public:
    BitboardSet pieces;
    CastlingRights rights;
    Color::Value turn{Color::WHITE};
    uint8_t enPassantIndex{UINT8_MAX};
};
