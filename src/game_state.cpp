#include "game_state.hpp"
#include <algorithm>

namespace ddz
{

    void applyMove(GameState &g, const Combo &c)
    {
        if (!c.valid() || c.cards.empty())
        {
            g.cur = (g.cur + 1) % 3;
            return;
        }
        auto &h = g.hands[g.cur];
        for (int card : c.cards)
        {
            auto it = std::find(h.begin(), h.end(), card);
            if (it != h.end())
                h.erase(it);
        }
        for (int card : c.cards)
            g.discard.push_back(card);
        g.played += (int)c.cards.size();
        g.table = c;
        g.tablePlayer = g.cur;
        if (h.empty())
        {
            g.over = true;
            g.winner = (g.cur == g.landlord) ? 0 : 1;
            return;
        }
        g.cur = (g.cur + 1) % 3;
    }

    double handStrength(const std::vector<int> &h)
    {
        int cnt[18] = {0};
        for (int c : h)
            cnt[pointOf(c)]++;
        double s = 0;
        for (int p = 3; p <= 17; p++)
        {
            if (cnt[p] == 4)
                s += 6;
            else if (p >= 15)
                s += cnt[p] * 2.0;
            else if (p >= 14)
                s += cnt[p] * 1.0;
            else if (p >= 12)
                s += cnt[p] * 0.5;
        }
        if (cnt[17])
            s += 4;
        if (cnt[16])
            s += 3;
        return s;
    }

} // namespace ddz