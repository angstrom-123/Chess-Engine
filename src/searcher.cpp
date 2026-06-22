#include "searcher.h"

#include <chrono>
#include <immintrin.h>
#include <iostream>

void Searcher::FindBest(const BoardState& state, uint64_t ms)
{
    auto startPoint = std::chrono::high_resolution_clock::now();

    // TODO: The actual search

    FindMoves(std::forward<const BoardState>(state));

    std::cout << "Found " << m_MoveBuffer.Size() << " moves" << std::endl;
    // for (const auto& move : m_MoveBuffer) {
    //     std::cout << "Move from " << static_cast<uint64_t>(move.from) << " to " << static_cast<uint64_t>(move.to) << " with " << Piece::Show(move.piece) << std::endl;
    // }
}

void Searcher::FindMoves(const BoardState& state)
{
    m_MoveBuffer.Clear();

    for (uint8_t i = 0; i < 64; i++) {
        const auto [color, piece] = state.pieces.PieceInSquare(i);
        if (color == state.turn) {
            switch (piece) {
                case Piece::PAWN:
                    FindPawnMoves(i, std::forward<const BoardState>(state));
                    break;
                case Piece::KNIGHT:
                    FindKnightMoves(i, std::forward<const BoardState>(state));
                    break;
                case Piece::BISHOP:
                    FindBishopMoves(i, std::forward<const BoardState>(state));
                    break;
                case Piece::ROOK:
                    FindRookMoves(i, std::forward<const BoardState>(state));
                    break;
                case Piece::QUEEN:
                    FindQueenMoves(i, std::forward<const BoardState>(state));
                    break;
                case Piece::KING:
                    FindKingMoves(i, std::forward<const BoardState>(state));
                    break;
                default:
                    break;
            }
        }
    }
}

bool Searcher::SquareUnderAttack(uint64_t bit, Color::Value color, const BoardState& state)
{
    Bitboard attacks = 0;
    for (uint8_t i = 0; i < Piece::MAX_ENUM; i++) {
        Bitboard mask = state.pieces.Get(color, static_cast<Piece::Value>(i));
        while (mask) {
            uint8_t index = _tzcnt_u64(mask);
            attacks |= m_AttackTable.GetAttacks(index, static_cast<Piece::Value>(i), color, state.pieces.OccupancyMask());
            mask &= (mask - 1);
        }

        if (attacks & bit)
            return true;
    }
    return false;
}

bool Searcher::IsLegal(const Move& move, const BoardState& state)
{
    // Simulate move
    BoardState candidateState = state;
    candidateState.turn = Color::Opposite(state.turn);
    candidateState.pieces.UnsetAll(Color::Opposite(state.turn), move.to);
    candidateState.pieces.Unset(state.turn, move.piece, move.from);
    candidateState.pieces.Set(state.turn, Piece::IsValid(move.promote) ? move.promote : move.piece, move.to);

    // Check if friendly king is under attack
    uint64_t kingBit = candidateState.pieces.Get(state.turn, Piece::KING);
    bool res = !SquareUnderAttack(kingBit, candidateState.turn, std::forward<const BoardState>(state));
    return res;
}

void Searcher::FindPawnMoves(uint8_t index, const BoardState& state)
{
    Buffer<Move, 12> pseudoLegal;

    const uint64_t backRankMask = 0xFF000000000000FF; // Same for both colors because pawns can't go back
    const uint64_t homeSquareMask = (state.turn == Color::WHITE) ? 0xFF000000000000 : 0x000000000000FF00;

    // In all cases, need to check if the move is a promotion

    // Attacks, check that target square contains enemy piece
    {
        Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::PAWN, state.turn, state.pieces.OccupancyMask());
        attacks &= ~state.pieces.OccupancyMask(state.turn);
        attacks &= state.pieces.OccupancyMask(Color::Opposite(state.turn));
        
        while (attacks) {
            uint8_t toIndex = _tzcnt_u64(attacks);
            uint64_t toBit = 1ul << toIndex;
            if (toBit & backRankMask) {
                // Promotion
                pseudoLegal.EmplaceBack(index, toIndex, Piece::PAWN, Piece::KNIGHT);
                pseudoLegal.EmplaceBack(index, toIndex, Piece::PAWN, Piece::BISHOP);
                pseudoLegal.EmplaceBack(index, toIndex, Piece::PAWN, Piece::ROOK);
                pseudoLegal.EmplaceBack(index, toIndex, Piece::PAWN, Piece::QUEEN);
            } else {
                pseudoLegal.EmplaceBack(index, toIndex, Piece::PAWN, Piece::Invalid());
            }
            attacks &= (attacks - 1);
        }
    }

    // Single / Double push
    {
        int8_t delta = (state.turn == Color::WHITE) ? -8 : 8;
        uint8_t toIndex = index + delta;
        uint64_t toBit = 1ul << toIndex;
        if (!state.pieces.Has(toIndex)) {
            if (toBit & backRankMask) {
                // Promotion
                pseudoLegal.EmplaceBack(index, toIndex, Piece::PAWN, Piece::KNIGHT);
                pseudoLegal.EmplaceBack(index, toIndex, Piece::PAWN, Piece::BISHOP);
                pseudoLegal.EmplaceBack(index, toIndex, Piece::PAWN, Piece::ROOK);
                pseudoLegal.EmplaceBack(index, toIndex, Piece::PAWN, Piece::QUEEN);
            } else {
                pseudoLegal.EmplaceBack(index, toIndex, Piece::PAWN, Piece::Invalid());
                toIndex += delta;
                if (((1ul << index) & homeSquareMask) && !state.pieces.Has(toIndex)) {
                    pseudoLegal.EmplaceBack(index, toIndex, Piece::PAWN, Piece::Invalid());
                }
            }
        }
    }

    for (const auto& move : pseudoLegal) {
        if (IsLegal(move, std::forward<const BoardState>(state)))
            m_MoveBuffer.PushBack(move);
    }
}

void Searcher::FindKnightMoves(uint8_t index, const BoardState& state)
{
    Buffer<Move, 8> pseudoLegal;

    Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::KNIGHT, state.turn, 0);
    attacks &= ~state.pieces.OccupancyMask(state.turn);

    while (attacks) {
        uint8_t toIndex = _tzcnt_u64(attacks);
        pseudoLegal.EmplaceBack(index, toIndex, Piece::KNIGHT, Piece::Invalid());
        attacks &= (attacks - 1);
    }

    for (const auto& move : pseudoLegal) {
        if (IsLegal(move, std::forward<const BoardState>(state)))
            m_MoveBuffer.PushBack(move);
    }
}

void Searcher::FindKingMoves(uint8_t index, const BoardState& state)
{
    Buffer<Move, 8> pseudoLegal;

    // Attacks
    {
        Bitboard attacks = m_AttackTable.GetAttacks(index, Piece::KING, state.turn, 0);
        attacks &= ~state.pieces.OccupancyMask(state.turn);

        while (attacks) {
            uint8_t toIndex = _tzcnt_u64(attacks);
            pseudoLegal.EmplaceBack(index, toIndex, Piece::KING, Piece::Invalid());
            attacks &= (attacks - 1);
        }
    }

    // Castling
    {
        // NOTE: Only checking that king is safe at start and intermediate squares, since the 
        //       end square will be checked alongside all other moves before inserting into 
        //       the move buffer.
        if (!SquareUnderAttack(1ul << index, Color::Opposite(state.turn), std::forward<const BoardState>(state))) {
            if (state.rights.kingside[state.turn] && !state.pieces.Has(index + 1) &&
                    !SquareUnderAttack(1ul << (index + 1), Color::Opposite(state.turn), std::forward<const BoardState>(state)))
                pseudoLegal.EmplaceBack(index, index + 2, Piece::KING, Piece::Invalid());

            if (state.rights.queenside[state.turn] && !state.pieces.Has(index - 1) && 
                    !SquareUnderAttack(1ul << (index - 1), Color::Opposite(state.turn), std::forward<const BoardState>(state)))
                pseudoLegal.EmplaceBack(index, index - 2, Piece::KING, Piece::Invalid());
        }
    }

    for (const auto& move : pseudoLegal) {
        if (IsLegal(move, std::forward<const BoardState>(state)))
            m_MoveBuffer.PushBack(move);
    }
}

void Searcher::FindBishopMoves(uint8_t index, const BoardState& state)
{
    FindSliderMoves(index, Piece::BISHOP, std::forward<const BoardState>(state));
}

void Searcher::FindRookMoves(uint8_t index, const BoardState& state)
{
    FindSliderMoves(index, Piece::ROOK, std::forward<const BoardState>(state));
}

void Searcher::FindQueenMoves(uint8_t index, const BoardState& state)
{
    FindSliderMoves(index, Piece::QUEEN, std::forward<const BoardState>(state));
}

void Searcher::FindSliderMoves(uint8_t index, Piece::Value piece, const BoardState& state)
{
    Buffer<Move, 32> pseudoLegal;

    Bitboard attacks = m_AttackTable.GetAttacks(index, piece, state.turn, state.pieces.OccupancyMask());
    attacks &= ~state.pieces.OccupancyMask(state.turn);

    while (attacks) {
        uint8_t toIndex = _tzcnt_u64(attacks);
        pseudoLegal.EmplaceBack(index, toIndex, piece, Piece::Invalid());
        attacks &= (attacks - 1);
    }

    for (const auto& move : pseudoLegal) {
        if (IsLegal(move, std::forward<const BoardState>(state)))
            m_MoveBuffer.PushBack(move);
    }
}
