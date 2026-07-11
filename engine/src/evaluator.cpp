#include "evaluator.h"

int64_t Evaluator::Evaluate(const BoardState& state)
{
    int64_t eval = 0;

    eval += EvaluateMaterialBalance(std::forward<const BoardState>(state));

    return eval;
}

int64_t Evaluator::EvaluateMaterialBalance(const BoardState& state)
{
    int64_t materialEval = 0;

    Color::Value friendly = state.turn;
    Color::Value enemy = Color::Opposite(state.turn);

    int64_t values[Piece::MAX_ENUM];
    values[Piece::PAWN] = 100;
    values[Piece::KNIGHT] = 300;
    values[Piece::BISHOP] = 310;
    values[Piece::ROOK] = 500;
    values[Piece::QUEEN] = 900;
    values[Piece::KING] = 0; // Not counted towards material balance

    int64_t nPawns = state.pieces.Count(friendly, Piece::PAWN) - state.pieces.Count(enemy, Piece::PAWN);
    int64_t nKnights = state.pieces.Count(friendly, Piece::KNIGHT) - state.pieces.Count(enemy, Piece::KNIGHT);
    int64_t nBishops = state.pieces.Count(friendly, Piece::BISHOP) - state.pieces.Count(enemy, Piece::BISHOP);
    int64_t nRooks = state.pieces.Count(friendly, Piece::ROOK) - state.pieces.Count(enemy, Piece::ROOK);
    int64_t nQueens = state.pieces.Count(friendly, Piece::QUEEN) - state.pieces.Count(enemy, Piece::QUEEN);

    materialEval = nPawns * values[Piece::PAWN]
            + nKnights * values[Piece::KNIGHT]
            + nBishops * values[Piece::BISHOP]
            + nRooks * values[Piece::ROOK]
            + nQueens * values[Piece::QUEEN];

    return materialEval;
}
