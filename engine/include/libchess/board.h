#pragma once

#include <cstdint>
#include <string>

#include "boardState.h"
#include "core.h"
#include "move.h"
#include "searcher.h"

namespace libchess {
    class Board {
    public:
        Board(const char *fen);
        Move Go(uint64_t moveMs);
        void MakeMove(LongAlgebraicMove lan);
        bool HasError();
        const char *GetError();
        void Show(std::string& result);

    private:
        void Clear();
        bool SplitFEN(const char *fen, uint64_t length, FenView (&views)[13]);

    private:
        EngineState::Value m_InternalState{EngineState::OK};
        Searcher m_Searcher;
        BoardState m_State;
        uint64_t m_FullMoves{1};
    };
}
