#include "features.hpp"
#include "game_state.hpp"
#include <vector>

namespace ddz
{
    std::vector<double> stateFeat(const GameState &g, int p)
    {
        std::vector<double> f(STATE_DIM, 0.0);
        int i = 0;

        int myCnt[18] = {0};
        for (int c : g.hands[p])
            myCnt[pointOf(c)]++;
        for (int r = 3; r <= 17; r++)
            f[i++] = myCnt[r] / 4.0;

        int seen[18] = {0};
        for (int c : g.discard)
            seen[pointOf(c)]++;
        for (int r = 3; r <= 17; r++)
            f[i++] = seen[r] / 4.0;

        for (int q = 0; q < 3; q++)
            f[i++] = g.hands[q].size() / 20.0;

        f[i++] = isFree(g, p) ? 1.0 : 0.0;
        f[i++] = (p == g.landlord) ? 1.0 : 0.0;
        f[i++] = g.played / 54.0;
        f[i++] = g.discard.size() / 54.0;
        f[i++] = (g.table.valid()) ? 1.0 : 0.0;

        int ctIdx = (int)g.table.type;
        if (ctIdx >= 0 && ctIdx < 15)
            f[i + ctIdx] = 1.0;
        i += 15;
        return f;
    }

    std::vector<double> actionFeat(const GameState &g, int p, const Combo &c)
    {
        std::vector<double> f(ACTION_DIM, 0.0);
        if (!c.valid())
        {
            f[15] = 1.0;
            return f;
        }
        int ct = (int)c.type;
        if (ct >= 0 && ct < 15)
            f[ct] = 1.0;
        f[15] = 0.0;
        f[16] = c.mainRank / 17.0;
        f[17] = c.len / 12.0;
        f[18] = c.cards.size() / 20.0;
        f[19] = (c.type == ComboType::Bomb || c.type == ComboType::Rocket) ? 1.0 : 0.0;
        return f;
    }

    std::vector<double> joinedFeat(const GameState &g, int p, const Combo &c)
    {
        std::vector<double> s = stateFeat(g, p);
        std::vector<double> a = actionFeat(g, p, c);
        s.insert(s.end(), a.begin(), a.end());
        return s;
    }
} // namespace ddz
