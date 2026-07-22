#include "searcher.h"
#include "move.h"
#include "evaluator.h"
#include "movegen.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <immintrin.h>
#include <iostream>

namespace chrono = std::chrono;

Move Searcher::FindBest(const BoardState& state, uint64_t msRemaining)
{
    auto startPoint = chrono::high_resolution_clock::now();
    uint64_t startMs = chrono::time_point_cast<chrono::milliseconds>(startPoint).time_since_epoch().count();

    uint64_t targetTimeMs = msRemaining / std::max(10ul, static_cast<uint64_t>(50 - std::floor(state.halfMoves / 2)));
    uint64_t targetMs = startMs + targetTimeMs;

    BoardState workingState(state);

    m_NodesSearched = 0;
    m_NodesEvaluated = 0;
    m_SearchAborted = false;

    // Iterative deepening
    Move bestMove = Move::Invalid();
    uint8_t depth = 0;
    while (++depth) {
        // Only check termination condition every 2048 nodes to save expensive clock calls
        if (m_NodesSearched & 2048) {
            auto nowPoint = chrono::high_resolution_clock::now();
            uint64_t nowMs = chrono::time_point_cast<chrono::milliseconds>(nowPoint).time_since_epoch().count();
            if (nowMs >= targetMs)
                break;
        }

        AttackMoveBuffer attackMoves;
        movegen::FindAttacks(std::forward<const BoardState>(state), m_AttackTable, attackMoves);

        QuietMoveBuffer quietMoves;
        movegen::FindQuiets(std::forward<const BoardState>(state), m_AttackTable, quietMoves);

        CombinedMoveBuffer moves;
        OrderMoves(bestMove, attackMoves, quietMoves, moves);

        LineBuffer principalVariation(depth);
        LineBuffer localLine(depth);

        Move depthBestMove = Move::Invalid();
        int64_t bestScore = -INT64_MAX;
        int64_t alpha = -INT64_MAX;
        int64_t beta = INT64_MAX;

        for (Move move : moves) {
            m_NodesSearched++;
            MoveData moveData = MakeMove(move, workingState);
            if (!WasLegal(moveData, workingState)) {
                UnmakeMove(moveData, workingState);
                continue;
            }
            m_NodesEvaluated++;

            localLine.Resize(0);
            int64_t score = -Search((SearchInfo) {
                .state = workingState, 
                .alpha = -beta, 
                .beta = -alpha, 
                .depth = static_cast<uint8_t>(depth - 1), 
                .ply = 1,
                .pv = localLine,
                .targetMs = targetMs
            });
            UnmakeMove(moveData, workingState);

            // Ran out of time
            if (m_SearchAborted)
                break;

            if (score > bestScore) {
                bestScore = score;
                depthBestMove = move;
                if (score > alpha)
                    alpha = score;

                principalVariation[0] = move;
                for (uint8_t i = 0; i < localLine.Size(); i++)
                    principalVariation[i + 1] = localLine[i];
                principalVariation.Resize(localLine.Size() + 1);
            }
        }

        // Ran out of time - Incomplete search so don't save result
        if (m_SearchAborted)
            break;

        // Checkmate or stalemate
        if (!Move::IsValid(depthBestMove)) {
            std::cout << "Couldn't find a valid move" << std::endl;
            break;
        }

        // Show principal variation at this depth
        std::cout << "Principal Variation (depth=" << static_cast<int>(depth) << "): ";
        for (const auto move : principalVariation)
            std::cout << move.ToLAN().chars << ", ";
        std::cout << std::endl;

        bestMove = depthBestMove;
    }

    // TODO: Optimisation from iterative deepening?
    
    auto endPoint = chrono::high_resolution_clock::now();
    uint64_t endMs = chrono::time_point_cast<chrono::milliseconds>(endPoint).time_since_epoch().count();

    uint64_t msTaken = endMs - startMs;
    float mnpsSearch = static_cast<float>(m_NodesSearched) / static_cast<float>(msTaken) / 1000.0;
    float mnpsEvaluate = static_cast<float>(m_NodesEvaluated) / static_cast<float>(msTaken) / 1000.0;
    std::cout << "Searched " << m_NodesSearched << " nodes in " << msTaken << "ms (" << std::setprecision(3) << mnpsSearch << " million nps)" <<  std::endl;
    std::cout << "  of those Quiesced " << m_NodesQuiesced << " nodes" << std::endl;
    std::cout << "Evaluated " << m_NodesEvaluated << " nodes in " << msTaken << "ms (" << std::setprecision(3) << mnpsEvaluate << " million nps)" <<  std::endl;

    return bestMove;
}

int64_t Searcher::Search(SearchInfo&& info)
{
    // Only check termination condition every 2048 nodes to save expensive clock calls
    if (m_NodesSearched & 2048) {
        auto nowPoint = chrono::high_resolution_clock::now();
        uint64_t nowMs = chrono::time_point_cast<chrono::milliseconds>(nowPoint).time_since_epoch().count();
        if (nowMs >= info.targetMs) {
            m_SearchAborted = true;
            return 0;
        }
    }

    info.pv.Resize(0);

    if (info.depth == 0) 
        return Quiesce((QuiesceInfo) {
            .state = info.state, 
            .alpha = info.alpha, 
            .beta = info.beta, 
            .ply = 0,
            .targetMs = info.targetMs
        });

    int64_t max = -INT64_MAX;
    LineBuffer localLine(info.depth);

    AttackMoveBuffer attackMoves;
    movegen::FindAttacks(std::forward<const BoardState>(info.state), m_AttackTable, attackMoves);

    QuietMoveBuffer quietMoves;
    movegen::FindQuiets(std::forward<const BoardState>(info.state), m_AttackTable, quietMoves);

    CombinedMoveBuffer moves;
    OrderMoves(Move::Invalid(), attackMoves, quietMoves, moves);

    for (Move move : moves) {
        m_NodesSearched++;
        MoveData moveData = MakeMove(move, info.state);
        if (!WasLegal(moveData, info.state)) {
            UnmakeMove(moveData, info.state);
            continue;
        }
        m_NodesEvaluated++;

        localLine.Resize(0);
        int64_t score = -Search(SearchInfo::Next(std::forward<SearchInfo&&>(info), localLine));
        UnmakeMove(moveData, info.state);

        if (score > max) {
            max = score;
            if (score > info.alpha) {
                info.alpha = score;

                info.pv[0] = move;
                for (uint8_t i = 0; i < localLine.Size(); i++)
                    info.pv[i + 1] = localLine[i];
                info.pv.Resize(localLine.Size() + 1);
            }
        }

        if (score >= info.beta)
            return max;
    }

    if (max == -INT64_MAX) {
        // Checkmate
        Bitboard king = info.state.pieces.OccupancyMask(info.state.turn, Piece::KING);
        if (SquareUnderAttack(king, Color::Opposite(info.state.turn), info.state)) 
            return -MATE_EVAL + info.ply;

        // Stalemate
        return 0;
    }
    return max;
}

int64_t Searcher::Quiesce(QuiesceInfo&& info)
{
    // Only check termination condition every 2048 nodes to save expensive clock calls
    if (m_NodesSearched & 2048) {
        auto nowPoint = chrono::high_resolution_clock::now();
        uint64_t nowMs = chrono::time_point_cast<chrono::milliseconds>(nowPoint).time_since_epoch().count();
        if (nowMs >= info.targetMs) {
            m_SearchAborted = true;
            return 0;
        }
    }

    int64_t staticEval = Evaluator::Evaluate(info.state);

    // Stand Pat
    int64_t max = staticEval;
    if (max >= info.beta)
        return max;
    if (max > info.alpha)
        info.alpha = max;

    AttackMoveBuffer attackMoves;
    movegen::FindAttacks(info.state, m_AttackTable, attackMoves);
    for (Move move : attackMoves) {
        m_NodesSearched++;
        m_NodesQuiesced++;
        MoveData moveData = MakeMove(move, info.state);
        if (!WasLegal(moveData, info.state)) {
            UnmakeMove(moveData, info.state);
            continue;
        }
        m_NodesEvaluated++;

        int64_t score = -Quiesce(QuiesceInfo::Next(std::forward<QuiesceInfo&&>(info)));
        UnmakeMove(moveData, info.state);

        if (score >= info.beta)
            return score;

        if (score > max)
            max = score;

        if (score > info.alpha)
            info.alpha = score;
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

// TODO: Improve this
void Searcher::OrderMoves(Move bestMove, const AttackMoveBuffer& attacks, const QuietMoveBuffer& quiets, CombinedMoveBuffer& ordered)
{
    if (Move::IsValid(bestMove))
        ordered.PushBack(bestMove);

    for (const Move move : attacks) {
        if (move != bestMove)
            ordered.PushBack(move);
    }

    for (const Move move : quiets) {
        if (move != bestMove)
            ordered.PushBack(move);
    }
}
