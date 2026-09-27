#include "combo.hpp"
#include <algorithm>
#include <cstring>

namespace ddz
{

    const char *ctNameEN(ComboType t)
    {
        switch (t)
        {
        case ComboType::Single:
            return "Single";
        case ComboType::Pair:
            return "Pair";
        case ComboType::Triple:
            return "Triple";
        case ComboType::TripleSingle:
            return "Triple+1";
        case ComboType::TriplePair:
            return "Triple+2";
        case ComboType::Straight:
            return "Straight";
        case ComboType::PairStraight:
            return "PairRun";
        case ComboType::Plane:
            return "Plane";
        case ComboType::PlaneSingle:
            return "Plane+1";
        case ComboType::PlanePair:
            return "Plane+2";
        case ComboType::FourTwoSingle:
            return "Four+2";
        case ComboType::FourTwoPair:
            return "Four+2P";
        case ComboType::Bomb:
            return "BOMB";
        case ComboType::Rocket:
            return "ROCKET";
        default:
            return "Invalid";
        }
    }

    std::string comboName(const Combo &c)
    {
        if (!c.valid())
            return "Pass";
        std::string s = ctNameEN(c.type);
        if (c.type == ComboType::Single || c.type == ComboType::Pair || c.type == ComboType::Triple ||
            c.type == ComboType::TripleSingle || c.type == ComboType::TriplePair ||
            c.type == ComboType::Straight || c.type == ComboType::PairStraight ||
            c.type == ComboType::Plane || c.type == ComboType::PlaneSingle ||
            c.type == ComboType::PlanePair || c.type == ComboType::Bomb)
            s += std::string(" ") + pointName(c.mainRank);
        return s;
    }

    Combo parseCombo(const std::vector<int> &cards)
    {
        Combo c;
        c.cards = cards;
        int n = (int)cards.size();
        if (n == 0)
            return c;
        int cnt[18] = {0};
        for (int x : cards)
            cnt[pointOf(x)]++;
        int pts[18], np = 0;
        for (int p = 3; p <= 17; p++)
            if (cnt[p])
                pts[np++] = p;

        if (n == 2 && cnt[16] == 1 && cnt[17] == 1)
        {
            c.type = ComboType::Rocket;
            c.mainRank = 17;
            return c;
        }
        if (n == 4 && np == 1)
        {
            c.type = ComboType::Bomb;
            c.mainRank = pts[0];
            return c;
        }
        if (n == 1)
        {
            c.type = ComboType::Single;
            c.mainRank = pts[0];
            return c;
        }
        if (n == 2 && np == 1)
        {
            c.type = ComboType::Pair;
            c.mainRank = pts[0];
            return c;
        }
        if (n == 3 && np == 1)
        {
            c.type = ComboType::Triple;
            c.mainRank = pts[0];
            return c;
        }
        if (n == 4 && np == 2)
        {
            if (cnt[pts[0]] == 3)
            {
                c.type = ComboType::TripleSingle;
                c.mainRank = pts[0];
                return c;
            }
            if (cnt[pts[1]] == 3)
            {
                c.type = ComboType::TripleSingle;
                c.mainRank = pts[1];
                return c;
            }
        }
        if (n == 5 && np == 2)
        {
            if (cnt[pts[0]] == 3 && cnt[pts[1]] == 2)
            {
                c.type = ComboType::TriplePair;
                c.mainRank = pts[0];
                return c;
            }
            if (cnt[pts[0]] == 2 && cnt[pts[1]] == 3)
            {
                c.type = ComboType::TriplePair;
                c.mainRank = pts[1];
                return c;
            }
        }
        if (n >= 5 && np == n)
        {
            bool ok = true;
            for (int i = 0; i < np; i++)
                if (pts[i] > 14)
                {
                    ok = false;
                    break;
                }
            if (ok)
                for (int i = 1; i < np; i++)
                    if (pts[i] != pts[i - 1] + 1)
                    {
                        ok = false;
                        break;
                    }
            if (ok)
            {
                c.type = ComboType::Straight;
                c.mainRank = pts[np - 1];
                c.len = n;
                return c;
            }
        }
        if (n >= 6 && n % 2 == 0 && np == n / 2)
        {
            bool ok = true;
            for (int i = 0; i < np; i++)
                if (cnt[pts[i]] != 2 || pts[i] > 14)
                {
                    ok = false;
                    break;
                }
            if (ok)
                for (int i = 1; i < np; i++)
                    if (pts[i] != pts[i - 1] + 1)
                    {
                        ok = false;
                        break;
                    }
            if (ok)
            {
                c.type = ComboType::PairStraight;
                c.mainRank = pts[np - 1];
                c.len = np;
                return c;
            }
        }
        if (n >= 6 && n % 3 == 0 && np == n / 3)
        {
            bool ok = true;
            for (int i = 0; i < np; i++)
                if (cnt[pts[i]] != 3 || pts[i] > 14)
                {
                    ok = false;
                    break;
                }
            if (ok)
                for (int i = 1; i < np; i++)
                    if (pts[i] != pts[i - 1] + 1)
                    {
                        ok = false;
                        break;
                    }
            if (ok)
            {
                c.type = ComboType::Plane;
                c.mainRank = pts[np - 1];
                c.len = np;
                return c;
            }
        }
        {
            int tri[18], nt = 0;
            for (int i = 0; i < np; i++)
                if (cnt[pts[i]] >= 3 && pts[i] <= 14)
                    tri[nt++] = pts[i];
            for (int k = 2; k <= 6; k++)
            {
                if (4 * k == n)
                {
                    for (int i = 0; i + k <= nt; i++)
                    {
                        bool ok = true;
                        for (int j = 1; j < k; j++)
                            if (tri[i + j] != tri[i + j - 1] + 1)
                            {
                                ok = false;
                                break;
                            }
                        if (!ok)
                            continue;
                        int rem[18];
                        memcpy(rem, cnt, sizeof(rem));
                        for (int j = 0; j < k; j++)
                            rem[tri[i] + j] -= 3;
                        int tot = 0;
                        for (int q = 3; q <= 17; q++)
                            tot += rem[q];
                        if (tot == k)
                        {
                            c.type = ComboType::PlaneSingle;
                            c.mainRank = tri[i] + k - 1;
                            c.len = k;
                            return c;
                        }
                    }
                }
                if (5 * k == n)
                {
                    for (int i = 0; i + k <= nt; i++)
                    {
                        bool ok = true;
                        for (int j = 1; j < k; j++)
                            if (tri[i + j] != tri[i + j - 1] + 1)
                            {
                                ok = false;
                                break;
                            }
                        if (!ok)
                            continue;
                        int rem[18];
                        memcpy(rem, cnt, sizeof(rem));
                        for (int j = 0; j < k; j++)
                            rem[tri[i] + j] -= 3;
                        int tot = 0;
                        bool good = true;
                        for (int q = 3; q <= 17; q++)
                        {
                            if (rem[q] == 0)
                                continue;
                            if (rem[q] != 2)
                            {
                                good = false;
                                break;
                            }
                            tot += 2;
                        }
                        if (good && tot == 2 * k)
                        {
                            c.type = ComboType::PlanePair;
                            c.mainRank = tri[i] + k - 1;
                            c.len = k;
                            return c;
                        }
                    }
                }
            }
        }
        if (n == 6)
            for (int i = 0; i < np; i++)
                if (cnt[pts[i]] == 4)
                {
                    c.type = ComboType::FourTwoSingle;
                    c.mainRank = pts[i];
                    return c;
                }
        if (n == 8 && np == 3)
            for (int i = 0; i < np; i++)
                if (cnt[pts[i]] == 4)
                {
                    bool good = true;
                    for (int j = 0; j < np; j++)
                        if (j != i && cnt[pts[j]] != 2)
                            good = false;
                    if (good)
                    {
                        c.type = ComboType::FourTwoPair;
                        c.mainRank = pts[i];
                        return c;
                    }
                }
        return c;
    }

    bool canBeat(const Combo &a, const Combo &b)
    {
        if (!a.valid())
            return false;
        if (!b.valid())
            return true;
        if (a.type == ComboType::Rocket)
            return true;
        if (b.type == ComboType::Rocket)
            return false;
        if (a.type == ComboType::Bomb && b.type != ComboType::Bomb)
            return true;
        if (b.type == ComboType::Bomb && a.type != ComboType::Bomb)
            return false;
        if (a.type != b.type)
            return false;
        if (a.len != b.len)
            return false;
        return a.mainRank > b.mainRank;
    }

    // ============================================================
    // 出牌生成
    // ============================================================
    std::vector<Combo> genAllMoves(const std::vector<int> &hand)
    {
        std::vector<Combo> res;
        std::vector<std::vector<int>> bp(18);
        for (int c : hand)
            bp[pointOf(c)].push_back(c);
        auto add = [&](const std::vector<int> &cd)
        {
            Combo c = parseCombo(cd);
            if (c.valid())
                res.push_back(c);
        };

        for (int p = 3; p <= 17; p++)
            if (!bp[p].empty())
                add({bp[p][0]});
        for (int p = 3; p <= 17; p++)
            if (bp[p].size() >= 2)
                add({bp[p][0], bp[p][1]});
        for (int p = 3; p <= 17; p++)
            if (bp[p].size() >= 3)
                add({bp[p][0], bp[p][1], bp[p][2]});
        for (int p = 3; p <= 17; p++)
            if (bp[p].size() == 4)
                add(bp[p]);
        if (!bp[16].empty() && !bp[17].empty())
            add({bp[16][0], bp[17][0]});

        for (int p = 3; p <= 17; p++)
            if (bp[p].size() >= 3)
                for (int q = 3; q <= 17; q++)
                    if (q != p && !bp[q].empty())
                        add({bp[p][0], bp[p][1], bp[p][2], bp[q][0]});
        for (int p = 3; p <= 17; p++)
            if (bp[p].size() >= 3)
                for (int q = 3; q <= 17; q++)
                    if (q != p && bp[q].size() >= 2)
                        add({bp[p][0], bp[p][1], bp[p][2], bp[q][0], bp[q][1]});

        for (int L = 5; L <= 12; L++)
            for (int s = 3; s + L - 1 <= 14; s++)
            {
                bool ok = true;
                for (int i = 0; i < L; i++)
                    if (bp[s + i].empty())
                    {
                        ok = false;
                        break;
                    }
                if (!ok)
                    continue;
                std::vector<int> cd;
                for (int i = 0; i < L; i++)
                    cd.push_back(bp[s + i][0]);
                add(cd);
            }
        for (int L = 3; L <= 10; L++)
            for (int s = 3; s + L - 1 <= 14; s++)
            {
                bool ok = true;
                for (int i = 0; i < L; i++)
                    if (bp[s + i].size() < 2)
                    {
                        ok = false;
                        break;
                    }
                if (!ok)
                    continue;
                std::vector<int> cd;
                for (int i = 0; i < L; i++)
                {
                    cd.push_back(bp[s + i][0]);
                    cd.push_back(bp[s + i][1]);
                }
                add(cd);
            }
        for (int L = 2; L <= 6; L++)
            for (int s = 3; s + L - 1 <= 14; s++)
            {
                bool ok = true;
                for (int i = 0; i < L; i++)
                    if (bp[s + i].size() < 3)
                    {
                        ok = false;
                        break;
                    }
                if (!ok)
                    continue;
                std::vector<int> mainC;
                std::vector<int> used(18, 0);
                for (int i = 0; i < L; i++)
                {
                    for (int j = 0; j < 3; j++)
                        mainC.push_back(bp[s + i][j]);
                    used[s + i] = 3;
                }
                {
                    std::vector<int> wing;
                    for (int q = 3; q <= 17 && (int)wing.size() < L; q++)
                        for (int i = used[q]; i < (int)bp[q].size() && (int)wing.size() < L; i++)
                            wing.push_back(bp[q][i]);
                    if ((int)wing.size() == L)
                    {
                        std::vector<int> cd = mainC;
                        cd.insert(cd.end(), wing.begin(), wing.end());
                        add(cd);
                    }
                }
                {
                    std::vector<int> wing;
                    for (int q = 3; q <= 17; q++)
                    {
                        if ((int)bp[q].size() - used[q] >= 2)
                        {
                            wing.push_back(bp[q][used[q]]);
                            wing.push_back(bp[q][used[q] + 1]);
                            if ((int)wing.size() >= 2 * L)
                                break;
                        }
                    }
                    if ((int)wing.size() == 2 * L)
                    {
                        std::vector<int> cd = mainC;
                        cd.insert(cd.end(), wing.begin(), wing.end());
                        add(cd);
                    }
                }
            }
        for (int p = 3; p <= 17; p++)
            if (bp[p].size() == 4)
            {
                std::vector<int> wing;
                for (int q = 3; q <= 17 && (int)wing.size() < 2; q++)
                {
                    if (q == p)
                        continue;
                    for (int i = 0; i < (int)bp[q].size() && (int)wing.size() < 2; i++)
                        wing.push_back(bp[q][i]);
                }
                if ((int)wing.size() == 2)
                {
                    std::vector<int> cd = bp[p];
                    cd.insert(cd.end(), wing.begin(), wing.end());
                    add(cd);
                }
                std::vector<int> w2;
                for (int q = 3; q <= 17; q++)
                {
                    if (q == p)
                        continue;
                    if (bp[q].size() >= 2)
                    {
                        w2.push_back(bp[q][0]);
                        w2.push_back(bp[q][1]);
                        if ((int)w2.size() >= 4)
                            break;
                    }
                }
                if ((int)w2.size() == 4)
                {
                    std::vector<int> cd = bp[p];
                    cd.insert(cd.end(), w2.begin(), w2.end());
                    add(cd);
                }
            }
        return res;
    }

    std::vector<Combo> genBeats(const std::vector<int> &hand, const Combo &last)
    {
        std::vector<Combo> res;
        std::vector<std::vector<int>> bp(18);
        for (int c : hand)
            bp[pointOf(c)].push_back(c);
        auto add = [&](const std::vector<int> &cd)
        {
            Combo c = parseCombo(cd);
            if (c.valid() && canBeat(c, last))
                res.push_back(c);
        };
        for (int p = 3; p <= 17; p++)
            if (bp[p].size() == 4)
                add(bp[p]);
        if (!bp[16].empty() && !bp[17].empty())
            add({bp[16][0], bp[17][0]});

        int need = last.mainRank, L = last.len;
        switch (last.type)
        {
        case ComboType::Single:
            for (int p = need + 1; p <= 17; p++)
                if (!bp[p].empty())
                    add({bp[p][0]});
            break;
        case ComboType::Pair:
            for (int p = need + 1; p <= 17; p++)
                if (bp[p].size() >= 2)
                    add({bp[p][0], bp[p][1]});
            break;
        case ComboType::Triple:
            for (int p = need + 1; p <= 17; p++)
                if (bp[p].size() >= 3)
                    add({bp[p][0], bp[p][1], bp[p][2]});
            break;
        case ComboType::TripleSingle:
            for (int p = need + 1; p <= 17; p++)
                if (bp[p].size() >= 3)
                    for (int q = 3; q <= 17; q++)
                        if (q != p && !bp[q].empty())
                        {
                            add({bp[p][0], bp[p][1], bp[p][2], bp[q][0]});
                            break;
                        }
            break;
        case ComboType::TriplePair:
            for (int p = need + 1; p <= 17; p++)
                if (bp[p].size() >= 3)
                    for (int q = 3; q <= 17; q++)
                        if (q != p && bp[q].size() >= 2)
                        {
                            add({bp[p][0], bp[p][1], bp[p][2], bp[q][0], bp[q][1]});
                            break;
                        }
            break;
        case ComboType::Straight:
            for (int s = 3; s + L - 1 <= 14; s++)
            {
                if (s + L - 1 <= need)
                    continue;
                bool ok = true;
                for (int i = 0; i < L; i++)
                    if (bp[s + i].empty())
                    {
                        ok = false;
                        break;
                    }
                if (!ok)
                    continue;
                std::vector<int> cd;
                for (int i = 0; i < L; i++)
                    cd.push_back(bp[s + i][0]);
                add(cd);
            }
            break;
        case ComboType::PairStraight:
            for (int s = 3; s + L - 1 <= 14; s++)
            {
                if (s + L - 1 <= need)
                    continue;
                bool ok = true;
                for (int i = 0; i < L; i++)
                    if (bp[s + i].size() < 2)
                    {
                        ok = false;
                        break;
                    }
                if (!ok)
                    continue;
                std::vector<int> cd;
                for (int i = 0; i < L; i++)
                {
                    cd.push_back(bp[s + i][0]);
                    cd.push_back(bp[s + i][1]);
                }
                add(cd);
            }
            break;
        case ComboType::Plane:
            for (int s = 3; s + L - 1 <= 14; s++)
            {
                if (s + L - 1 <= need)
                    continue;
                bool ok = true;
                for (int i = 0; i < L; i++)
                    if (bp[s + i].size() < 3)
                    {
                        ok = false;
                        break;
                    }
                if (!ok)
                    continue;
                std::vector<int> cd;
                for (int i = 0; i < L; i++)
                    for (int j = 0; j < 3; j++)
                        cd.push_back(bp[s + i][j]);
                add(cd);
            }
            break;
        case ComboType::PlaneSingle:
        case ComboType::PlanePair:
        {
            bool withPair = (last.type == ComboType::PlanePair);
            for (int s = 3; s + L - 1 <= 14; s++)
            {
                if (s + L - 1 <= need)
                    continue;
                bool ok = true;
                for (int i = 0; i < L; i++)
                    if (bp[s + i].size() < 3)
                    {
                        ok = false;
                        break;
                    }
                if (!ok)
                    continue;
                std::vector<int> mainC;
                std::vector<int> used(18, 0);
                for (int i = 0; i < L; i++)
                {
                    for (int j = 0; j < 3; j++)
                        mainC.push_back(bp[s + i][j]);
                    used[s + i] = 3;
                }
                std::vector<int> wing;
                if (withPair)
                {
                    for (int q = 3; q <= 17; q++)
                        if ((int)bp[q].size() - used[q] >= 2)
                        {
                            wing.push_back(bp[q][used[q]]);
                            wing.push_back(bp[q][used[q] + 1]);
                            if ((int)wing.size() >= 2 * L)
                                break;
                        }
                    if ((int)wing.size() != 2 * L)
                        continue;
                }
                else
                {
                    for (int q = 3; q <= 17 && (int)wing.size() < L; q++)
                        for (int i = used[q]; i < (int)bp[q].size() && (int)wing.size() < L; i++)
                            wing.push_back(bp[q][i]);
                    if ((int)wing.size() != L)
                        continue;
                }
                std::vector<int> cd = mainC;
                cd.insert(cd.end(), wing.begin(), wing.end());
                add(cd);
            }
            break;
        }
        case ComboType::FourTwoSingle:
            for (int p = need + 1; p <= 17; p++)
                if (bp[p].size() == 4)
                {
                    std::vector<int> wing;
                    for (int q = 3; q <= 17 && (int)wing.size() < 2; q++)
                    {
                        if (q == p)
                            continue;
                        for (int i = 0; i < (int)bp[q].size() && (int)wing.size() < 2; i++)
                            wing.push_back(bp[q][i]);
                    }
                    if ((int)wing.size() == 2)
                    {
                        std::vector<int> cd = bp[p];
                        cd.insert(cd.end(), wing.begin(), wing.end());
                        add(cd);
                    }
                }
            break;
        default:
            break;
        }
        return res;
    }

} // namespace ddz