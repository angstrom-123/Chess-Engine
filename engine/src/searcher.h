#pragma once

#include "boardState.h"
#include "buffer.h"
#include "core.h"
#include "move.h"
#include "attackTable.h"

const uint64_t MAX_PLY = 32;
using LineBuffer = Buffer<Move, MAX_PLY>;

struct MoveData {
    Move move{Move::Invalid()};
    Piece::Value capture{Piece::Invalid()};
    CastlingRights rights{0};
    Color::Value turn{Color::Invalid()};
    uint8_t enPassantIndex{0};
    uint64_t halfMoves{0};
};

class Searcher {
public:
    Move FindBest(const BoardState& state, uint64_t ms);
    MoveData MakeMove(Move move, BoardState& state);

private:
    void UnmakeMove(MoveData moveData, BoardState& state);
    int64_t Search(BoardState& state, int64_t alpha, int64_t beta, uint8_t depth, uint8_t ply, LineBuffer& pv);
    int64_t Quiesce(BoardState& state, int64_t alpha, int64_t beta, uint8_t ply);
    bool SquareUnderAttack(uint64_t bit, Color::Value color, const BoardState& state);
    bool WasLegal(MoveData moveData, const BoardState& state);

private:
    uint64_t m_NodesEvaluated{0};
    uint64_t m_NodesSearched{0};
    uint64_t m_NodesQuiesced{0};
    AttackTable m_AttackTable{AttackTable()};
};
