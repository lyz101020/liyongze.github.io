#pragma once
#include "game_state.hpp"
#include <vector>

namespace ddz
{

    constexpr int STATE_DIM = 53;
    constexpr int ACTION_DIM = 20;
    constexpr int INPUT_DIM = STATE_DIM + ACTION_DIM;

    std::vector<double> stateFeat(const GameState &g, int p);
    std::vector<double> actionFeat(const GameState &g, int p, const Combo &c);
    std::vector<double> joinedFeat(const GameState &g, int p, const Combo &c);

} // namespace ddz