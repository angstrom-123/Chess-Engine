#include "move.h"
#include <cmath>

LongAlgebraicMove Move::ToLAN()
{
    auto ToAlgebraic = [this](uint8_t index, char (&result)[2]) {
        uint8_t x = index % 8;
        uint8_t y = std::floor(index / 8);
        result[0] = 'a' + x;
        result[1] = '8' - y; // TODO: Check that this inversion is right
    };

    LongAlgebraicMove lan = {'\0'};

    ToAlgebraic(from, lan.from);
    ToAlgebraic(to, lan.to);

    if (Piece::IsValid(promote)) {
        switch (promote) {
            case Piece::KNIGHT:
                lan.promote = 'n';
                break;
            case Piece::BISHOP:
                lan.promote = 'b';
                break;
            case Piece::ROOK:
                lan.promote = 'r';
                break;
            case Piece::QUEEN:
                lan.promote = 'q';
                break;
            default:
                return LongAlgebraicMove::Invalid();
        }
    }

    return lan;
}
