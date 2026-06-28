#pragma once

#include "boardState.h"
#include "buffer.h"
#include "core.h"
#include "move.h"
#include "attackTable.h"

const uint64_t MAX_POSSIBLE_QUIETS = 240; // Should be enough, includes pseudolegal
const uint64_t MAX_POSSIBLE_ATTACKS = 112; // Upper bound, includes pseudolegal
using QuietMoveBuffer = Buffer<Move, MAX_POSSIBLE_QUIETS>;
using AttackMoveBuffer = Buffer<Move, MAX_POSSIBLE_ATTACKS>;

class Searcher {
public:
    Move FindBest(const BoardState& state, uint64_t ms);
    void MakeMove(Move move, BoardState& state);

private:
    int64_t Search(BoardState& state, int64_t alpha, int64_t beta, uint8_t depth);
    int64_t Evaluate(const BoardState& state);
    void FindAttacks(const BoardState& state, AttackMoveBuffer& attackBuffer);
    void FindQuiets(const BoardState& state, QuietMoveBuffer& quietBuffer);
    bool SquareUnderAttack(uint64_t bit, Color::Value color, const BoardState& state);
    bool IsLegal(Move move, BoardState& state);
    void FindPawnAttacks(uint8_t index, const BoardState& state, AttackMoveBuffer& attackBuffer);
    void FindPawnQuiets(uint8_t index, const BoardState& state, QuietMoveBuffer& quietBuffer);
    void FindKnightAttacks(uint8_t index, const BoardState& state, AttackMoveBuffer& attackBuffer);
    void FindKnightQuiets(uint8_t index, const BoardState& state, QuietMoveBuffer& quietBuffer);
    void FindKingAttacks(uint8_t index, const BoardState& state, AttackMoveBuffer& attackBuffer);
    void FindKingQuiets(uint8_t index, const BoardState& state, QuietMoveBuffer& quietBuffer);
    void FindBishopAttacks(uint8_t index, const BoardState& state, AttackMoveBuffer& attackBuffer);
    void FindBishopQuiets(uint8_t index, const BoardState& state, QuietMoveBuffer& quietBuffer);
    void FindRookAttacks(uint8_t index, const BoardState& state, AttackMoveBuffer& attackBuffer);
    void FindRookQuiets(uint8_t index, const BoardState& state, QuietMoveBuffer& quietBuffer);
    void FindQueenAttacks(uint8_t index, const BoardState& state, AttackMoveBuffer& attackBuffer);
    void FindQueenQuiets(uint8_t index, const BoardState& state, QuietMoveBuffer& quietBuffer);
    void FindSliderAttacks(uint8_t index, Piece::Value piece, const BoardState& state, AttackMoveBuffer& attackBuffer);
    void FindSliderQuiets(uint8_t index, Piece::Value piece, const BoardState& state, QuietMoveBuffer& quietBuffer);

private:
    uint64_t m_NodesEvaluated{0};
    uint64_t m_NodesSearched{0};
    AttackTable m_AttackTable{AttackTable()};
};
