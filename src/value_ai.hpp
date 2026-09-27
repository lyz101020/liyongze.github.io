#pragma once
#include "mlp.hpp"
#include <memory>
#include <random>
#include <string>

namespace ddz
{

    struct ReplayBuffer
    {
        struct Item
        {
            std::vector<double> phi;
            double target;
        };
        std::vector<Item> buf;
        int cap = 50000;
        int idx = 0;
        void push(const Item &it);
        void sample(std::vector<Item> &out, int n, std::mt19937 &rng) const;
        int size() const { return (int)buf.size(); }
    };

    struct ValueAI
    {
        MLP net;
        std::shared_ptr<ReplayBuffer> replay = std::make_shared<ReplayBuffer>();
        double lr = 0.001;
        long long updates = 0;
        double savedWinRate = 0.0;
        bool inited = false;

        void ensureInit(std::mt19937 &rng);
        int choose(const GameState &g, int p, const std::vector<Combo> &cands,
                   std::mt19937 &rng, bool explore, double eps = 0.10);
        void learn(const std::vector<std::pair<std::vector<double>, bool>> &decisions,
                   int winner, int landlord, std::mt19937 &rng);
    };

    bool saveWeights(const ValueAI &ai, const std::string &path, double winRate = -1.0);
    bool loadWeights(ValueAI &ai, const std::string &path, double &outWinRate, std::mt19937 &rng);

} // namespace ddz