#pragma once

#include "core.h"
#include <utility>

using Bitboard = uint64_t;

class BitboardSet {
public:
    void StartPos();

    Bitboard Get(Color::Value color, Piece::Value piece) const;
    void Set(Color::Value color, Piece::Value piece, uint8_t index);
    void Unset(Color::Value color, Piece::Value piece, uint8_t index);
    void UnsetAll(Color::Value color, uint8_t index);
    void Clear();
    bool Has(Color::Value color, Piece::Value piece, uint8_t index) const;
    bool Has(Color::Value color, uint8_t index) const;
    bool Has(uint8_t index) const;
    std::pair<Color::Value, Piece::Value> PieceInSquare(uint8_t index) const;
    uint64_t OccupancyMask(Color::Value color) const;
    uint64_t OccupancyMask() const;

private:
    uint64_t m_Bits[Color::MAX_ENUM][Piece::MAX_ENUM]{0};
    uint64_t m_Combined[Color::MAX_ENUM]{0};
};
