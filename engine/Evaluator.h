#pragma once
#include "Game.h"
#include "MoveSearch.h"
#include <array>

namespace tetris {
class Evaluator {
public:
    static Placement getBestPlacementDepth2(const Game& game, const std::array<double, 9>& weights, MoveSearch& searcher);
};
}
