#pragma once 

#include "boardState.h"
#include <cstdint>
const int64_t MATE_EVAL = 10000000;

struct SEE {
    typedef enum {
        LOSING,
        WINNING,
        EQUAL
    } Value;
};

class Evaluator {
public:
    static int64_t Evaluate(const BoardState& state);

private:
    static int64_t MaterialBalance(const BoardState& state);
    static int64_t CastlingRightIncentive(const BoardState& state);
    static SEE::Value EvaluateStaticExchange(uint8_t index, const BoardState& state);
};
