#include "value_ai.hpp"
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>

namespace ddz
{
    void ReplayBuffer::push(const Item &it)
    {
        if ((int)buf.size() < cap)
            buf.push_back(it);
        else
        {
            buf[idx] = it;
            idx = (idx + 1) % cap;
        }
    }
    void ReplayBuffer::sample(std::vector<Item> &out, int n, std::mt19937 &rng) const
    {
        out.clear();
        if (buf.empty())
            return;
        n = std::min(n, (int)buf.size());
        std::uniform_int_distribution<int> d(0, (int)buf.size() - 1);
        for (int i = 0; i < n; i++)
            out.push_back(buf[d(rng)]);
    }

    void ValueAI::ensureInit(std::mt19937 &rng)
    {
        if (inited)
            return;
        net.init(rng);
        inited = true;
    }

    int ValueAI::choose(const GameState &g, int p, const std::vector<Combo> &cands, std::mt19937 &rng,
                        bool explore, double eps)
    {
        int n = (int)cands.size();
        if (n == 0)
            return -1;
        if (n == 1)
            return 0;

        if (explore && std::uniform_real_distribution<double>(0, 1)(rng) < eps)
            return std::uniform_int_distribution<int>(0, n - 1)(rng);

        double bestV = -1e9;
        std::vector<int> bests;
        for (int i = 0; i < n; i++)
        {
            std::vector<double> phi = joinedFeat(g, p, cands[i]);
            double v = net.forward(phi);
            if (v > bestV + 1e-9)
            {
                bestV = v;
                bests.clear();
                bests.push_back(i);
            }
            else if (v >= bestV - 1e-9)
                bests.push_back(i);
        }
        return bests[std::uniform_int_distribution<int>(0, (int)bests.size() - 1)(rng)];
    }

    void ValueAI::learn(const std::vector<std::pair<std::vector<double>, bool>> &decisions,
                        int winner, int landlord, std::mt19937 &rng)
    {
        for (auto &d : decisions)
        {
            double z = (winner == (d.second ? 0 : 1)) ? 1.0 : 0.0;
            replay->push({d.first, z});
        }
        if (replay->size() < 256)
            return;
        std::vector<ReplayBuffer::Item> batch;
        replay->sample(batch, 256, rng);
        for (auto &it : batch)
        {
            net.forward(it.phi);
            net.backward(it.target, lr);
            updates++;
        }
    }

    // ============================================================
    // 权重存取
    // ============================================================
    bool saveWeights(const ValueAI &ai, const std::string &path, double winRate)
    {
        std::ofstream f(path);
        if (!f)
            return false;
        double wr = (winRate >= 0) ? winRate : ai.savedWinRate;
        f << wr << "\n"
          << ai.updates << "\n";
        f << ai.net.inD << " " << ai.net.hidD << "\n";
        for (int i = 0; i < ai.net.hidD; i++)
        {
            for (int j = 0; j < ai.net.inD; j++)
                f << ai.net.W1[i][j] << " ";
            f << ai.net.b1[i] << "\n";
        }
        for (int i = 0; i < ai.net.hidD; i++)
            f << ai.net.W2[i] << " ";
        f << ai.net.b2 << "\n";
        return true;
    }

    bool loadWeights(ValueAI &ai, const std::string &path, double &outWinRate, std::mt19937 &rng)
    {
        outWinRate = 0.0;
        std::ifstream f(path);
        if (!f)
            return false;

        double wr;
        long long upd;
        if (!(f >> wr >> upd))
            return false;
        int inD, hidD;
        if (!(f >> inD >> hidD))
            return false;
        if (inD != INPUT_DIM || hidD != HID_DIM)
        {
            std::cerr << "[loadWeights] dim mismatch (" << inD << "x" << hidD
                      << " vs " << INPUT_DIM << "x" << HID_DIM << "). Ignored.\n";
            return false;
        }
        ai.ensureInit(rng);
        for (int i = 0; i < hidD; i++)
        {
            for (int j = 0; j < inD; j++)
                if (!(f >> ai.net.W1[i][j]))
                    return false;
            if (!(f >> ai.net.b1[i]))
                return false;
        }
        for (int i = 0; i < hidD; i++)
            if (!(f >> ai.net.W2[i]))
                return false;
        if (!(f >> ai.net.b2))
            return false;

        ai.updates = upd;
        ai.savedWinRate = wr;
        outWinRate = wr;
        return true;
    }

} // namespace ddz