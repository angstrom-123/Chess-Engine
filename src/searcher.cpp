#include "searcher.h"

#include <chrono>
#include <cstdint>
#include <ctime>
#include <immintrin.h>
#include <iomanip>
#include <iostream>

Move Searcher::FindBest(const BoardState& state, uint64_t ms)
{
    // Non-const version for making and unmaking moves
    BoardState workingState(state);

    // TODO: Track time
    auto startPoint = std::chrono::high_resolution_clock::now();

    m_NodesSearched = 0;
    m_NodesEvaluated = 0;

    AttackMoveBuffer attackMoves;
    FindAttacks(std::forward<const BoardState>(state), attackMoves);
    std::cout << "Found " << attackMoves.Size() << " attacks" << std::endl;

    const uint8_t depth = 7;

    int64_t bestScore = -INT64_MAX;
    Move bestMove = Move::Invalid();
    for (Move move : attackMoves) {
        m_NodesSearched++;
        if (!IsLegal(move, workingState))
            continue;
        m_NodesEvaluated++;
        BoardState candidateState(state);
        MakeMove(move, candidateState);
        int64_t score = -Search(candidateState, -INT64_MAX, INT64_MAX, depth - 1);
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
    }

    QuietMoveBuffer quietMoves;
    FindQuiets(std::forward<const BoardState>(state), quietMoves);
    std::cout << "Found " << quietMoves.Size() << " quiets" << std::endl;
    for (Move move : quietMoves) {
        m_NodesSearched++;
        if (!IsLegal(move, workingState))
            continue;
        m_NodesEvaluated++;
        BoardState candidateState(state);
        MakeMove(move, candidateState);
        int64_t score = -Search(candidateState, -INT64_MAX, INT64_MAX, depth - 1);
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
    }

    if (!Move::IsValid(bestMove))
        std::cout << "Couldn't find a valid move" << std::endl;
    
    auto endPoint = std::chrono::high_resolution_clock::now();

    uint64_t startMs = std::chrono::time_point_cast<std::chrono::milliseconds>(startPoint).time_since_epoch().count();
    uint64_t endMs = std::chrono::time_point_cast<std::chrono::milliseconds>(endPoint).time_since_epoch().count();

    uint64_t msTaken = endMs - startMs;
    float mnpsSearch = static_cast<float>(m_NodesSearched) / static_cast<float>(msTaken) / 1000.0;
    float mnpsEvaluate = static_cast<float>(m_NodesEvaluated) / static_cast<float>(msTaken) / 1000.0;
    std::cout << "Searched " << m_NodesSearched << " nodes in " << msTaken << "ms (" << std::setprecision(3) << mnpsSearch << " million nps)" <<  std::endl;
    std::cout << "Evaluated " << m_NodesEvaluated << " nodes in " << msTaken << "ms (" << std::setprecision(3) << mnpsEvaluate << " million nps)" <<  std::endl;

    return bestMove;
}

int64_t Searcher::Search(BoardState& state, int64_t alpha, int64_t beta, uint8_t depth)
{
    if (depth == 0)
        return Evaluate(state);

    int64_t max = -INT64_MAX;

    AttackMoveBuffer attackMoves;
    FindAttacks(std::forward<const BoardState>(state), attackMoves);
    for (Move move : attackMoves) {
        m_NodesSearched++;
        if (!IsLegal(move, state))
            continue;
        m_NodesEvaluated++;
        BoardState candidateState(state);
        MakeMove(move, candidateState);
        int64_t score = -Search(candidateState, -beta, -alpha, depth - 1);
        if (score > max) {
            max = score;
            if (score > alpha)
                alpha = score;
        }
        if (score >= beta)
            return max;
    }

    QuietMoveBuffer quietMoves;
    FindQuiets(std::forward<const BoardState>(state), quietMoves);
    for (Move move : quietMoves) {
        m_NodesSearched++;
        if (!IsLegal(move, state))
            continue;
        m_NodesEvaluated++;
        BoardState candidateState(state);
        MakeMove(move, candidateState);
        int64_t score = -Search(candidateState, -beta, -alpha, depth - 1);
        if (score > max) {
            max = score;
            if (score > alpha)
                alpha = score;
        }
        if (score >= beta)
            return max;
    }
    return max;
}

void Searcher::MakeMove(Move move, BoardState& state)
{
    Color::Value friendly = state.turn;
    Color::Value enemy = Color::Opposite(state.turn);

    // Piece positions
    {
        state.pieces.Unset(friendly, move.GetPiece(), move.from);
        state.pieces.UnsetAll(enemy, move.to);
        state.pieces.Set(friendly, Piece::IsValid(move.GetPromote()) ? move.GetPromote() : move.GetPiece(), move.to);
    }

    // Castling rights
    {
        if (move.GetPiece() == Piece::KING) {
            state.rights &= ~CastlingRight::Kingside(friendly);
            state.rights &= ~CastlingRight::Queenside(friendly);
        } else if (move.GetPiece() == Piece::ROOK) {
            uint8_t kingsideIndex = (friendly == Color::WHITE) ? 63 : 7;
            uint8_t queensideIndex = (friendly == Color::WHITE) ? 56 : 0;

            if (move.from == kingsideIndex)
                state.rights &= ~CastlingRight::Kingside(friendly);

            if (move.from == queensideIndex)
                state.rights &= ~CastlingRight::Queenside(friendly);
        }
    }

    // En Passant
    {
        state.enPassantIndex = UINT8_MAX;
        if (move.GetPiece() == Piece::PAWN && Difference(move.from, move.to) == 16)
            state.enPassantIndex = (friendly == Color::WHITE) ? move.to + 8 : move.to - 8;
    }

    // Turn
    state.turn = enemy;

    // Half move counter
    state.halfMoves++;
}

int64_t Searcher::Evaluate(const BoardState& state)
{
    int64_t eval = 0;

    Color::Value friendly = state.turn;
    Color::Value enemy = Color::Opposite(state.turn);

    // Material balance
    {
        int64_t materialEval = 0;

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

        eval += materialEval;
    }

    return eval;
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

    for (uint8_t i = 0; i < 64; i++) {
        const auto [color, piece] = state.pieces.PieceInSquare(i);
        if (color != state.turn) 
            continue;

        switch (piece) {
            case Piece::PAWN:
                FindPawnQuiets(i, std::forward<const BoardState>(state), quietBuffer);
                break;
            case Piece::KNIGHT:
                FindKnightQuiets(i, std::forward<const BoardState>(state), quietBuffer);
                break;
            case Piece::BISHOP:
                FindBishopQuiets(i, std::forward<const BoardState>(state), quietBuffer);
                break;
            case Piece::ROOK:
                FindRookQuiets(i, std::forward<const BoardState>(state), quietBuffer);
                break;
            case Piece::QUEEN:
                FindQueenQuiets(i, std::forward<const BoardState>(state), quietBuffer);
                break;
            case Piece::KING:
                FindKingQuiets(i, std::forward<const BoardState>(state), quietBuffer);
                break;
            default:
                break;
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

bool Searcher::IsLegal(Move move, BoardState& state)
{
    // Simulate move
    BoardState candidateState = state;
    candidateState.turn = Color::Opposite(state.turn);
    candidateState.pieces.UnsetAll(Color::Opposite(state.turn), move.to);
    candidateState.pieces.Unset(state.turn, move.GetPiece(), move.from);
    candidateState.pieces.Set(state.turn, Piece::IsValid(move.GetPromote()) ? move.GetPromote() : move.GetPiece(), move.to);

    // Check if friendly king is under attack
    uint64_t kingBit = candidateState.pieces.OccupancyMask(state.turn, Piece::KING);
    if (kingBit == 0)
        return false;
    bool res = !SquareUnderAttack(kingBit, candidateState.turn, std::forward<const BoardState>(state));
    return res;
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
        // NOTE: Only checking that king is safe at start and intermediate squares, since the 
        //       end square will be checked alongside all other moves before inserting into 
        //       the move buffer.
        if (!SquareUnderAttack(1ul << index, Color::Opposite(state.turn), std::forward<const BoardState>(state))) {
            if ((state.rights & CastlingRight::Kingside(state.turn)) && !state.pieces.Has(index + 1) &&
                    !SquareUnderAttack(1ul << (index + 1), Color::Opposite(state.turn), std::forward<const BoardState>(state)))
                quietBuffer.EmplaceBack(index, index + 2, Piece::KING, Piece::Invalid());

            if ((state.rights & CastlingRight::Queenside(state.turn)) && !state.pieces.Has(index - 1) && 
                    !SquareUnderAttack(1ul << (index - 1), Color::Opposite(state.turn), std::forward<const BoardState>(state)))
                quietBuffer.EmplaceBack(index, index - 2, Piece::KING, Piece::Invalid());
        }
        // TODO: If this improved performance, defer this check too
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
