#include "searcher.h"
#include "move.h"
#include "evaluator.h"

#include <chrono>
#include <cstdint>
#include <ctime>
#include <immintrin.h>
#include <iomanip>
#include <iostream>

const uint8_t DEPTH = 7;
const uint8_t MAX_QUIESCENCE_PLY = 6;

// TODO: Make unmake
Move Searcher::FindBest(const BoardState& state, uint64_t ms)
{
    // TODO: Track time
    auto startPoint = std::chrono::high_resolution_clock::now();

    BoardState workingState(state);

    m_NodesSearched = 0;
    m_NodesEvaluated = 0;

    AttackMoveBuffer attackMoves;
    FindAttacks(std::forward<const BoardState>(state), attackMoves);
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
    FindQuiets(std::forward<const BoardState>(state), quietMoves);
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
    FindAttacks(std::forward<const BoardState>(state), attackMoves);
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
    FindQuiets(std::forward<const BoardState>(state), quietMoves);
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
    FindAttacks(state, attackMoves);
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

void Searcher::FindAttacks(const BoardState& state, AttackMoveBuffer& attackBuffer)
{
    attackBuffer.Clear();

    // Pawns
    {
        Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::PAWN);
        while (occupancy) {
            uint8_t index = _tzcnt_u64(occupancy);
            FindPawnAttacks(index, std::forward<const BoardState>(state), attackBuffer);
            occupancy &= (occupancy - 1);
        }
    }

    // Knights
    {
        Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::KNIGHT);
        while (occupancy) {
            uint8_t index = _tzcnt_u64(occupancy);
            FindKnightAttacks(index, std::forward<const BoardState>(state), attackBuffer);
            occupancy &= (occupancy - 1);
        }
    }

    // Bishops
    {
        Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::BISHOP);
        while (occupancy) {
            uint8_t index = _tzcnt_u64(occupancy);
            FindBishopAttacks(index, std::forward<const BoardState>(state), attackBuffer);
            occupancy &= (occupancy - 1);
        }
    }

    // Rooks
    {
        Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::ROOK);
        while (occupancy) {
            uint8_t index = _tzcnt_u64(occupancy);
            FindRookAttacks(index, std::forward<const BoardState>(state), attackBuffer);
            occupancy &= (occupancy - 1);
        }
    }

    // Queens
    {
        Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::QUEEN);
        while (occupancy) {
            uint8_t index = _tzcnt_u64(occupancy);
            FindQueenAttacks(index, std::forward<const BoardState>(state), attackBuffer);
            occupancy &= (occupancy - 1);
        }
    }

    // Kings
    {
        Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::KING);
        while (occupancy) {
            uint8_t index = _tzcnt_u64(occupancy);
            FindKingAttacks(index, std::forward<const BoardState>(state), attackBuffer);
            occupancy &= (occupancy - 1);
        }
    }
}

void Searcher::FindQuiets(const BoardState& state, QuietMoveBuffer& quietBuffer)
{
    quietBuffer.Clear();

    // Pawns
    {
        Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::PAWN);
        while (occupancy) {
            uint8_t index = _tzcnt_u64(occupancy);
            FindPawnQuiets(index, std::forward<const BoardState>(state), quietBuffer);
            occupancy &= (occupancy - 1);
        }
    }

    // Knights
    {
        Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::KNIGHT);
        while (occupancy) {
            uint8_t index = _tzcnt_u64(occupancy);
            FindKnightQuiets(index, std::forward<const BoardState>(state), quietBuffer);
            occupancy &= (occupancy - 1);
        }
    }

    // Bishops
    {
        Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::BISHOP);
        while (occupancy) {
            uint8_t index = _tzcnt_u64(occupancy);
            FindBishopQuiets(index, std::forward<const BoardState>(state), quietBuffer);
            occupancy &= (occupancy - 1);
        }
    }

    // Rooks
    {
        Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::ROOK);
        while (occupancy) {
            uint8_t index = _tzcnt_u64(occupancy);
            FindRookQuiets(index, std::forward<const BoardState>(state), quietBuffer);
            occupancy &= (occupancy - 1);
        }
    }

    // Queens
    {
        Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::QUEEN);
        while (occupancy) {
            uint8_t index = _tzcnt_u64(occupancy);
            FindQueenQuiets(index, std::forward<const BoardState>(state), quietBuffer);
            occupancy &= (occupancy - 1);
        }
    }

    // Kings
    {
        Bitboard occupancy = state.pieces.OccupancyMask(state.turn, Piece::KING);
        while (occupancy) {
            uint8_t index = _tzcnt_u64(occupancy);
            FindKingQuiets(index, std::forward<const BoardState>(state), quietBuffer);
            occupancy &= (occupancy - 1);
        }
    }
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

    // Should be impossible, but good to double check
    if (_mm_popcnt_u64(king) != 1)
        return false;

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

void Searcher::FindPawnAttacks(uint8_t index, const BoardState& state, AttackMoveBuffer& attackBuffer)
{
    const uint64_t backRankMask = 0xFF000000000000FF; // Same for both colors because pawns can't go back

    Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::PAWN, state.turn, state.pieces.OccupancyMask());
    attacks &= ~state.pieces.OccupancyMask(state.turn);
    attacks &= state.pieces.OccupancyMask(Color::Opposite(state.turn));
    
    while (attacks) {
        uint8_t toIndex = _tzcnt_u64(attacks);
        uint64_t toBit = 1ul << toIndex;
        if (toBit & backRankMask) {
            // Promotion
            attackBuffer.EmplaceBack(index, toIndex, Piece::PAWN, Piece::KNIGHT);
            attackBuffer.EmplaceBack(index, toIndex, Piece::PAWN, Piece::BISHOP);
            attackBuffer.EmplaceBack(index, toIndex, Piece::PAWN, Piece::ROOK);
            attackBuffer.EmplaceBack(index, toIndex, Piece::PAWN, Piece::QUEEN);
        } else {
            attackBuffer.EmplaceBack(index, toIndex, Piece::PAWN, Piece::Invalid());
        }
        attacks &= (attacks - 1);
    }
}

void Searcher::FindPawnQuiets(uint8_t index, const BoardState& state, QuietMoveBuffer& quietBuffer)
{
    const uint64_t backRankMask = 0xFF000000000000FF; // Same for both colors because pawns can't go back
    const uint64_t homeSquareMask = (state.turn == Color::WHITE) ? 0xFF000000000000 : 0x000000000000FF00;

    // In all cases, need to check if the move is a promotion

    int8_t delta = (state.turn == Color::WHITE) ? -8 : 8;
    uint8_t toIndex = index + delta;
    uint64_t toBit = 1ul << toIndex;
    if (!state.pieces.Has(toIndex)) {
        if (toBit & backRankMask) {
            // Promotion
            quietBuffer.EmplaceBack(index, toIndex, Piece::PAWN, Piece::KNIGHT);
            quietBuffer.EmplaceBack(index, toIndex, Piece::PAWN, Piece::BISHOP);
            quietBuffer.EmplaceBack(index, toIndex, Piece::PAWN, Piece::ROOK);
            quietBuffer.EmplaceBack(index, toIndex, Piece::PAWN, Piece::QUEEN);
        } else {
            quietBuffer.EmplaceBack(index, toIndex, Piece::PAWN, Piece::Invalid());
            toIndex += delta;
            if (((1ul << index) & homeSquareMask) && !state.pieces.Has(toIndex)) {
                quietBuffer.EmplaceBack(index, toIndex, Piece::PAWN, Piece::Invalid());
            }
        }
    }
}

void Searcher::FindKnightAttacks(uint8_t index, const BoardState& state, AttackMoveBuffer& attackBuffer)
{
    Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::KNIGHT, state.turn, 0);
    attacks &= ~state.pieces.OccupancyMask(state.turn);
    attacks &= state.pieces.OccupancyMask(Color::Opposite(state.turn));

    while (attacks) {
        uint8_t toIndex = _tzcnt_u64(attacks);
        attackBuffer.EmplaceBack(index, toIndex, Piece::KNIGHT, Piece::Invalid());
        attacks &= (attacks - 1);
    }
}

void Searcher::FindKnightQuiets(uint8_t index, const BoardState& state, QuietMoveBuffer& quietBuffer)
{
    Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::KNIGHT, state.turn, 0);
    attacks &= ~state.pieces.OccupancyMask();

    while (attacks) {
        uint8_t toIndex = _tzcnt_u64(attacks);
        quietBuffer.EmplaceBack(index, toIndex, Piece::KNIGHT, Piece::Invalid());
        attacks &= (attacks - 1);
    }
}

void Searcher::FindKingAttacks(uint8_t index, const BoardState& state, AttackMoveBuffer& attackBuffer)
{
    Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::KING, state.turn, 0);
    attacks &= ~state.pieces.OccupancyMask(state.turn);
    attacks &= state.pieces.OccupancyMask(Color::Opposite(state.turn));

    while (attacks) {
        uint8_t toIndex = _tzcnt_u64(attacks);
        attackBuffer.EmplaceBack(index, toIndex, Piece::KING, Piece::Invalid());
        attacks &= (attacks - 1);
    }
}

void Searcher::FindKingQuiets(uint8_t index, const BoardState& state, QuietMoveBuffer& quietBuffer)
{
    // Attacks
    {
        Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::KING, state.turn, 0);
        attacks &= ~state.pieces.OccupancyMask();

        while (attacks) {
            uint8_t toIndex = _tzcnt_u64(attacks);
            quietBuffer.EmplaceBack(index, toIndex, Piece::KING, Piece::Invalid());
            attacks &= (attacks - 1);
        }
    }

    // Castling
    {
        if ((state.rights & CastlingRight::Kingside(state.turn)) && !state.pieces.Has(index + 1) && !state.pieces.Has(index + 2))
            quietBuffer.EmplaceBack(index, index + 2, Piece::KING, Piece::Invalid());

        if ((state.rights & CastlingRight::Queenside(state.turn)) && !state.pieces.Has(index - 1) && !state.pieces.Has(index - 2))
            quietBuffer.EmplaceBack(index, index - 2, Piece::KING, Piece::Invalid());
    }
}

void Searcher::FindBishopAttacks(uint8_t index, const BoardState& state, AttackMoveBuffer& attackBuffer)
{
    FindSliderAttacks(index, Piece::BISHOP, std::forward<const BoardState>(state), attackBuffer);
}

void Searcher::FindBishopQuiets(uint8_t index, const BoardState& state, QuietMoveBuffer& quietBuffer)
{
    FindSliderQuiets(index, Piece::BISHOP, std::forward<const BoardState>(state), quietBuffer);
}

void Searcher::FindRookAttacks(uint8_t index, const BoardState& state, AttackMoveBuffer& attackBuffer)
{
    FindSliderAttacks(index, Piece::ROOK, std::forward<const BoardState>(state), attackBuffer);
}

void Searcher::FindRookQuiets(uint8_t index, const BoardState& state, QuietMoveBuffer& quietBuffer)
{
    FindSliderQuiets(index, Piece::ROOK, std::forward<const BoardState>(state), quietBuffer);
}

void Searcher::FindQueenAttacks(uint8_t index, const BoardState& state, AttackMoveBuffer& attackBuffer)
{
    FindSliderAttacks(index, Piece::QUEEN, std::forward<const BoardState>(state), attackBuffer);
}

void Searcher::FindQueenQuiets(uint8_t index, const BoardState& state, QuietMoveBuffer& quietBuffer)
{
    FindSliderQuiets(index, Piece::QUEEN, std::forward<const BoardState>(state), quietBuffer);
}

void Searcher::FindSliderAttacks(uint8_t index, Piece::Value piece, const BoardState& state, AttackMoveBuffer& attackBuffer)
{
    Bitboard attacks = m_AttackTable.GetAttacks(index, piece, state.turn, state.pieces.OccupancyMask());
    attacks &= ~state.pieces.OccupancyMask(state.turn);
    attacks &= state.pieces.OccupancyMask(Color::Opposite(state.turn));

    while (attacks) {
        uint8_t toIndex = _tzcnt_u64(attacks);
        attackBuffer.EmplaceBack(index, toIndex, piece, Piece::Invalid());
        attacks &= (attacks - 1);
    }
}

void Searcher::FindSliderQuiets(uint8_t index, Piece::Value piece, const BoardState& state, QuietMoveBuffer& quietBuffer)
{
    Bitboard attacks = m_AttackTable.GetAttacks(index, piece, state.turn, state.pieces.OccupancyMask());
    attacks &= ~state.pieces.OccupancyMask();

    while (attacks) {
        uint8_t toIndex = _tzcnt_u64(attacks);
        quietBuffer.EmplaceBack(index, toIndex, piece, Piece::Invalid());
        attacks &= (attacks - 1);
    }
}
