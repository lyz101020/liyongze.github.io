#pragma once
#include "combo.hpp"
#include <vector>

namespace ddz
{

    struct GameState
    {
        std::vector<int> hands[3];
        int landlord = 0;
        int cur = 0;
        int tablePlayer = -1;
        Combo table;
        int played = 0;
        bool over = false;
        int winner = -1;
        std::vector<int> discard;
    };

    inline bool isFree(const GameState &g, int p)
    {
        return g.tablePlayer == -1 || g.tablePlayer == p;
    }

    void applyMove(GameState &g, const Combo &c);
    double handStrength(const std::vector<int> &h);

} // namespace ddz