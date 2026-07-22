#include "evaluator.h"

int64_t Evaluator::Evaluate(const BoardState& state)
{
    int64_t eval = 0;

    eval += MaterialBalance(std::forward<const BoardState>(state));
    eval += CastlingRightIncentive(std::forward<const BoardState>(state));

    return eval;
}

int64_t Evaluator::MaterialBalance(const BoardState& state)
{
    int64_t materialEval = 0;

    Color::Value friendly = state.turn;
    Color::Value enemy = Color::Opposite(state.turn);

    int64_t nPawns = state.pieces.Count(friendly, Piece::PAWN) - state.pieces.Count(enemy, Piece::PAWN);
    int64_t nKnights = state.pieces.Count(friendly, Piece::KNIGHT) - state.pieces.Count(enemy, Piece::KNIGHT);
    int64_t nBishops = state.pieces.Count(friendly, Piece::BISHOP) - state.pieces.Count(enemy, Piece::BISHOP);
    int64_t nRooks = state.pieces.Count(friendly, Piece::ROOK) - state.pieces.Count(enemy, Piece::ROOK);
    int64_t nQueens = state.pieces.Count(friendly, Piece::QUEEN) - state.pieces.Count(enemy, Piece::QUEEN);

    materialEval = (nPawns * 100) + (nKnights * 300) + (nBishops * 300) + (nRooks * 500) + (nQueens * 900);

    return materialEval;
}

int64_t Evaluator::CastlingRightIncentive(const BoardState& state)
{
    int64_t castlingRightEval = 0;

    // Kingside
    uint8_t shift = (state.turn == Color::BLACK) ? 1 : 0;
    castlingRightEval += ((state.rights & (1 << shift)) >> shift) * 30;

    // Queenside
    shift += 2;
    castlingRightEval += ((state.rights & (1 << shift)) >> shift) * 20;

    return castlingRightEval;
}
