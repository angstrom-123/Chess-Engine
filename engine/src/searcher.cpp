#include "searcher.h"
#include "move.h"
#include "evaluator.h"
#include "movegen.h"

#include <chrono>
#include <cstdint>
#include <ctime>
#include <immintrin.h>
#include <iomanip>
#include <iostream>

const uint8_t DEPTH = 7;
const uint8_t MAX_QUIESCENCE_PLY = 6;

Move Searcher::FindBest(const BoardState& state, uint64_t ms)
{
    // TODO: Track time
    auto startPoint = std::chrono::high_resolution_clock::now();

    BoardState workingState(state);

    m_NodesSearched = 0;
    m_NodesEvaluated = 0;

    AttackMoveBuffer attackMoves;
    movegen::FindAttacks(std::forward<const BoardState>(state), m_AttackTable, attackMoves);
    std::cout << "Found " << attackMoves.Size() << " attacks" << std::endl;

    const uint8_t depth = DEPTH;
    LineBuffer principalVariation(depth);
    LineBuffer localLine(depth);

    Move bestMove = Move::Invalid();
    int64_t bestScore = -INT64_MAX;
    int64_t alpha = -INT64_MAX;
    int64_t beta = INT64_MAX;

    for (Move move : attackMoves) {
        m_NodesSearched++;
        MoveData moveData = MakeMove(move, workingState);
        if (!WasLegal(moveData, workingState)) {
            UnmakeMove(moveData, workingState);
            continue;
        }
        m_NodesEvaluated++;

        localLine.Resize(0);
        int64_t score = -Search(workingState, -beta, -alpha, depth - 1, 1, localLine);
        UnmakeMove(moveData, workingState);

        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
            if (score > alpha)
                alpha = score;

            principalVariation[0] = move;
            for (uint8_t i = 0; i < localLine.Size(); i++)
                principalVariation[i + 1] = localLine[i];
            principalVariation.Resize(localLine.Size() + 1);
        }
    }

    QuietMoveBuffer quietMoves;
    movegen::FindQuiets(std::forward<const BoardState>(state), m_AttackTable, quietMoves);
    std::cout << "Found " << quietMoves.Size() << " quiets" << std::endl;
    for (Move move : quietMoves) {
        m_NodesSearched++;
        MoveData moveData = MakeMove(move, workingState);
        if (!WasLegal(moveData, workingState)) {
            UnmakeMove(moveData, workingState);
            continue;
        }
        m_NodesEvaluated++;

        localLine.Resize(0);
        int64_t score = -Search(workingState, -beta, -alpha, depth - 1, 1, localLine);
        UnmakeMove(moveData, workingState);

        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
            if (score > alpha)
                alpha = score;

            principalVariation[0] = move;
            for (uint8_t i = 0; i < localLine.Size(); i++)
                principalVariation[i + 1] = localLine[i];
            principalVariation.Resize(localLine.Size() + 1);
        }
    }

    if (!Move::IsValid(bestMove))
        std::cout << "Couldn't find a valid move" << std::endl;

    std::cout << std::endl << "Principal Variation: ";
    for (const auto move : principalVariation)
        std::cout << move.ToLAN().chars << ", ";
    std::cout << std::endl;
    
    auto endPoint = std::chrono::high_resolution_clock::now();

    uint64_t startMs = std::chrono::time_point_cast<std::chrono::milliseconds>(startPoint).time_since_epoch().count();
    uint64_t endMs = std::chrono::time_point_cast<std::chrono::milliseconds>(endPoint).time_since_epoch().count();

    uint64_t msTaken = endMs - startMs;
    float mnpsSearch = static_cast<float>(m_NodesSearched) / static_cast<float>(msTaken) / 1000.0;
    float mnpsEvaluate = static_cast<float>(m_NodesEvaluated) / static_cast<float>(msTaken) / 1000.0;
    std::cout << "Searched " << m_NodesSearched << " nodes in " << msTaken << "ms (" << std::setprecision(3) << mnpsSearch << " million nps)" <<  std::endl;
    std::cout << "  of those Quiesced " << m_NodesQuiesced << " nodes" << std::endl;
    std::cout << "Evaluated " << m_NodesEvaluated << " nodes in " << msTaken << "ms (" << std::setprecision(3) << mnpsEvaluate << " million nps)" <<  std::endl;

    return bestMove;
}

int64_t Searcher::Search(BoardState& state, int64_t alpha, int64_t beta, uint8_t depth, uint8_t ply, LineBuffer& pv)
{
    pv.Resize(0);

    if (depth == 0)
        return Quiesce(state, alpha, beta, 0);

    int64_t max = -INT64_MAX;
    LineBuffer localLine(depth);

    AttackMoveBuffer attackMoves;
    movegen::FindAttacks(std::forward<const BoardState>(state), m_AttackTable, attackMoves);
    for (Move move : attackMoves) {
        m_NodesSearched++;
        MoveData moveData = MakeMove(move, state);
        if (!WasLegal(moveData, state)) {
            UnmakeMove(moveData, state);
            continue;
        }
        m_NodesEvaluated++;

        localLine.Resize(0);
        int64_t score = -Search(state, -beta, -alpha, depth - 1, ply + 1, localLine);
        UnmakeMove(moveData, state);

        if (score > max) {
            max = score;
            if (score > alpha) {
                alpha = score;

                pv[0] = move;
                for (uint8_t i = 0; i < localLine.Size(); i++)
                    pv[i + 1] = localLine[i];
                pv.Resize(localLine.Size() + 1);
            }
        }

        if (score >= beta)
            return max;
    }

    QuietMoveBuffer quietMoves;
    movegen::FindQuiets(std::forward<const BoardState>(state), m_AttackTable, quietMoves);
    for (Move move : quietMoves) {
        m_NodesSearched++;
        MoveData moveData = MakeMove(move, state);
        if (!WasLegal(moveData, state)) {
            UnmakeMove(moveData, state);
            continue;
        }
        m_NodesEvaluated++;

        localLine.Resize(0);
        int64_t score = -Search(state, -beta, -alpha, depth - 1, ply + 1, localLine);
        UnmakeMove(moveData, state);

        if (score > max) {
            max = score;
            if (score > alpha) {
                alpha = score;

                pv[0] = move;
                for (uint8_t i = 0; i < localLine.Size(); i++)
                    pv[i + 1] = localLine[i];
                pv.Resize(localLine.Size() + 1);
            }
        }

        if (score >= beta)
            return max;
    }

    if (max == -INT64_MAX) {
        // Checkmate
        Bitboard king = state.pieces.OccupancyMask(state.turn, Piece::KING);
        if (SquareUnderAttack(king, Color::Opposite(state.turn), state)) 
            return -MATE_EVAL + ply;

        // Stalemate
        return 0;
    }
    return max;
}

int64_t Searcher::Quiesce(BoardState& state, int64_t alpha, int64_t beta, uint8_t ply)
{
    int64_t staticEval = Evaluator::Evaluate(state);
    if (ply == MAX_QUIESCENCE_PLY)
        return staticEval;

    // Stand Pat
    int64_t max = staticEval;
    if (max >= beta)
        return max;
    if (max > alpha)
        alpha = max;

    AttackMoveBuffer attackMoves;
    movegen::FindAttacks(state, m_AttackTable, attackMoves);
    for (Move move : attackMoves) {
        m_NodesSearched++;
        m_NodesQuiesced++;
        MoveData moveData = MakeMove(move, state);
        if (!WasLegal(moveData, state)) {
            UnmakeMove(moveData, state);
            continue;
        }
        m_NodesEvaluated++;

        int64_t score = -Quiesce(state, -beta, -alpha, ply + 1);
        UnmakeMove(moveData, state);

        if (score >= beta)
            return score;

        if (score > max)
            max = score;

        if (score > alpha)
            alpha = score;
    }

    return max;
}

MoveData Searcher::MakeMove(Move move, BoardState& state)
{
    Piece::Value capture = state.pieces.PieceInSquare(move.to).second;

    Color::Value friendly = state.turn;
    Color::Value enemy = Color::Opposite(state.turn);

    // For unmaking the move later
    MoveData moveData = {
        .move = move,
        .capture = capture,
        .rights = state.rights,
        .turn = state.turn,
        .enPassantIndex = state.enPassantIndex,
        .halfMoves = state.halfMoves
    };

    // Move piece
    state.pieces.Unset(friendly, move.piece, move.from);
    if (Piece::IsValid(move.promote))
        state.pieces.Set(friendly, move.promote, move.to);
    else
        state.pieces.Set(friendly, move.piece, move.to);

    // Remove capture
    if (Piece::IsValid(capture))
        state.pieces.Unset(enemy, capture, move.to);

    // Move rook if castling
    if (move.piece == Piece::KING && Difference(move.from, move.to) == 2) {
        if (move.from > move.to) {
            state.pieces.Unset(friendly, Piece::ROOK, move.from - 4);
            state.pieces.Set(friendly, Piece::ROOK, move.from - 1);
        } else {
            state.pieces.Unset(friendly, Piece::ROOK, move.from + 3);
            state.pieces.Set(friendly, Piece::ROOK, move.from + 1);
        }
    }

    // Remove pawn if en passant
    if (move.piece == Piece::PAWN && move.to == state.enPassantIndex)
        state.pieces.Unset(enemy, Piece::PAWN, (friendly == Color::WHITE) ? move.to + 8 : move.to - 8);

    // Avoid updating castling rights after both sides lose the right
    if (state.rights != 0) {
        // Remove castling rights if king moved
        if (move.piece == Piece::KING)
            state.rights = 0;

        // Remove castling rights if rook moved from start square
        if (move.piece == Piece::ROOK) {
            if (move.from == (friendly == Color::WHITE ? 63 : 7))
                state.rights &= ~CastlingRight::Kingside(friendly);
            else if (move.from == (friendly == Color::WHITE ? 56 : 0))
                state.rights &= ~CastlingRight::Queenside(friendly);
        }

        // Remove castling rights if rook captured on start square
        if (capture == Piece::ROOK) {
            if (move.to == (enemy == Color::WHITE ? 63 : 7))
                state.rights &= ~CastlingRight::Kingside(enemy);
            else if (move.from == (enemy == Color::WHITE ? 56 : 0))
                state.rights &= ~CastlingRight::Queenside(enemy);
        }
    }

    // Update en passant square if double pawn push
    if (move.piece == Piece::PAWN && Difference(move.from, move.to) == 16)
        state.enPassantIndex = (friendly == Color::WHITE) ? move.to + 8 : move.to - 8;
    else 
        state.enPassantIndex = UINT8_MAX;

    state.turn = enemy;
    state.halfMoves++;

    return moveData;
}

void Searcher::UnmakeMove(MoveData moveData, BoardState& state)
{
    const Move& move = moveData.move;

    Color::Value friendly = moveData.turn;
    Color::Value enemy = Color::Opposite(moveData.turn);

    // Replace moving piece
    state.pieces.Set(friendly, move.piece, move.from);
    if (Piece::IsValid(move.promote))
        state.pieces.Unset(friendly, move.promote, move.to);
    else
        state.pieces.Unset(friendly, move.piece, move.to);

    // Replace capture
    if (Piece::IsValid(moveData.capture))
        state.pieces.Set(enemy, moveData.capture, move.to);

    // Replace rook if castling
    if (move.piece == Piece::KING && Difference(move.from, move.to) == 2) {
        if (move.from > move.to) {
            state.pieces.Set(friendly, Piece::ROOK, move.from - 4);
            state.pieces.Unset(friendly, Piece::ROOK, move.from - 1);
        } else {
            state.pieces.Set(friendly, Piece::ROOK, move.from + 3);
            state.pieces.Unset(friendly, Piece::ROOK, move.from + 1);
        }
    }

    // Replace pawn if en passant
    if (move.piece == Piece::PAWN && move.to == moveData.enPassantIndex)
        state.pieces.Set(enemy, Piece::PAWN, (friendly == Color::WHITE) ? move.to + 8 : move.to - 8);

    // Update variables
    state.rights = moveData.rights;
    state.turn = moveData.turn;
    state.enPassantIndex = moveData.enPassantIndex;
    state.halfMoves = moveData.halfMoves;
}

bool Searcher::SquareUnderAttack(uint64_t bit, Color::Value color, const BoardState& state)
{
    uint8_t index = _tzcnt_u64(bit);
    Bitboard occupancy = state.pieces.OccupancyMask();

    // Pawns
    {
        Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::PAWN, Color::Opposite(color), occupancy);
        if (attacks & state.pieces.OccupancyMask(color, Piece::PAWN))
            return true;
    }

    // Knights
    {
        Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::KNIGHT, Color::Opposite(color), occupancy);
        if (attacks & state.pieces.OccupancyMask(color, Piece::KNIGHT))
            return true;
    }

    // Bishops
    {
        Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::BISHOP, Color::Opposite(color), occupancy);
        if (attacks & state.pieces.OccupancyMask(color, Piece::BISHOP))
            return true;
    }

    // Rooks
    {
        Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::ROOK, Color::Opposite(color), occupancy);
        if (attacks & state.pieces.OccupancyMask(color, Piece::ROOK))
            return true;
    }

    // Queens
    {
        Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::QUEEN, Color::Opposite(color), occupancy);
        if (attacks & state.pieces.OccupancyMask(color, Piece::QUEEN))
            return true;
    }

    // Kings
    {
        Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::KING, Color::Opposite(color), occupancy);
        if (attacks & state.pieces.OccupancyMask(color, Piece::KING))
            return true;
    }

    return false;
}

bool Searcher::WasLegal(MoveData moveData, const BoardState& state)
{
    Bitboard king = state.pieces.OccupancyMask(moveData.turn, Piece::KING);

    bool targetAttacked = SquareUnderAttack(king, Color::Opposite(moveData.turn), std::forward<const BoardState>(state));

    // Check intermediate and start squares if castling
    if (moveData.move.piece == Piece::KING && Difference(moveData.move.from, moveData.move.to) == 2) {
        bool startAttacked = SquareUnderAttack(1ul << moveData.move.from, Color::Opposite(moveData.turn), std::forward<const BoardState>(state));
        uint8_t midIndex = (moveData.move.from > moveData.move.to) ? moveData.move.from - 1 : moveData.move.from + 1;
        bool midAttacked = SquareUnderAttack(1ul << midIndex, Color::Opposite(moveData.turn), std::forward<const BoardState>(state));

        return !(targetAttacked || startAttacked || midAttacked);
    }

    return !targetAttacked;
}

