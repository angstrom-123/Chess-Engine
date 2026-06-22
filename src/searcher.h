#pragma once

#include "boardState.h"
#include "buffer.h"
#include "move.h"
#include "attackTable.h"

class Searcher {
public:
    void FindBest(const BoardState& state, uint64_t ms);

private:
    void FindMoves(const BoardState& state);
    bool SquareUnderAttack(uint64_t bit, Color::Value color, const BoardState& state);
    bool IsLegal(const Move& move, const BoardState& state);
    void FindPawnMoves(uint8_t index, const BoardState& state);
    void FindKnightMoves(uint8_t index, const BoardState& state);
    void FindKingMoves(uint8_t index, const BoardState& state);
    void FindBishopMoves(uint8_t index, const BoardState& state);
    void FindRookMoves(uint8_t index, const BoardState& state);
    void FindQueenMoves(uint8_t index, const BoardState& state);
    void FindSliderMoves(uint8_t index, Piece::Value piece, const BoardState& state);

private:
    AttackTable m_AttackTable{AttackTable()};
    Buffer<Move, 256> m_MoveBuffer;
};
