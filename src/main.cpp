#include "value_ai.hpp"
#include "game_gui.hpp"
#include "replay_gui.hpp"
#include "gui_common.hpp"
#include "features.hpp"
#include "combo.hpp"
#include "game_state.hpp"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <thread>

using namespace std;
using namespace ddz;

// 定义在 game_gui.hpp 里的全局标志，由 main 拥有
namespace ddz
{
    bool g_requestReplay = false;
}

// ============================================================
// 洗牌 + 定地主
// ============================================================
namespace
{

    void shuffleDeal(GameState &g, mt19937 &rng)
    {
        vector<int> deck(54);
        iota(deck.begin(), deck.end(), 0);
        shuffle(deck.begin(), deck.end(), rng);
        for (int i = 0; i < 3; i++)
        {
            g.hands[i].assign(deck.begin() + i * 17, deck.begin() + (i + 1) * 17);
            sortHand(g.hands[i]);
        }
        vector<int> bottom(deck.begin() + 51, deck.end());
        int best = 0;
        double bs = -1;
        for (int i = 0; i < 3; i++)
        {
            double s = handStrength(g.hands[i]);
            if (s > bs)
            {
                bs = s;
                best = i;
            }
        }
        g.landlord = best;
        for (int c : bottom)
            g.hands[best].push_back(c);
        sortHand(g.hands[best]);
        g.cur = g.landlord;
        g.tablePlayer = -1;
        g.table = Combo();
        g.played = 0;
        g.over = false;
        g.winner = -1;
        g.discard.clear();
    }

} // namespace

// ============================================================
// 评估
// ============================================================
struct EvalResult
{
    double overall = 0, asLandlord = 0, asFarmer = 0;
    int totalGames = 0, llGames = 0, farmerGames = 0;
};

EvalResult evaluateDetailed(ValueAI &ai, int games, mt19937 &rng)
{
    int wins = 0, valid = 0, llW = 0, llG = 0, fmW = 0, fmG = 0;
    for (int i = 0; i < games; i++)
    {
        GameState g;
        shuffleDeal(g, rng);
        const int aiSeat = 0;
        int step = 0;
        while (!g.over && step < 500)
        {
            step++;
            int p = g.cur;
            bool fr = isFree(g, p);
            auto cands = fr ? genAllMoves(g.hands[p]) : genBeats(g.hands[p], g.table);
            if (!fr)
                cands.push_back(Combo());
            if (cands.empty())
                break;

            int idx;
            if (p == aiSeat)
            {
                idx = ai.choose(g, p, cands, rng, false);
            }
            else
            {
                idx = uniform_int_distribution<int>(0, (int)cands.size() - 1)(rng);
            }
            applyMove(g, cands[idx]);
        }
        if (g.winner < 0)
            continue;
        valid++;
        bool aiWin = (g.winner == ((aiSeat == g.landlord) ? 0 : 1));
        if (aiWin)
            wins++;
        if (aiSeat == g.landlord)
        {
            llG++;
            if (aiWin)
                llW++;
        }
        else
        {
            fmG++;
            if (aiWin)
                fmW++;
        }
    }
    EvalResult r;
    r.totalGames = valid;
    r.overall = valid ? (double)wins / valid : 0.0;
    r.llGames = llG;
    r.farmerGames = fmG;
    r.asLandlord = llG ? (double)llW / llG : 0.0;
    r.asFarmer = fmG ? (double)fmW / fmG : 0.0;
    return r;
}

// ============================================================
// main
// ============================================================
int main()
{
    cout << unitbuf;
    ios::sync_with_stdio(false);

    unsigned seed = (unsigned)chrono::steady_clock::now().time_since_epoch().count() ^ (unsigned)chrono::system_clock::now().time_since_epoch().count();
    mt19937 rng(seed);

    ValueAI ai;
    ai.ensureInit(rng);

    cout << "===========================================\n";
    cout << "  Dou Dizhu AI  (modular build)\n";
    cout << "===========================================\n\n";

    const double TARGET_WR = 0.85;
    double loadedWR = 0.0;
    bool loaded = loadWeights(ai, "ddz_best.txt", loadedWR, rng);
    if (loaded)
    {
        cout << "[Loaded] saved win-rate: " << (loadedWR * 100.0) << "%\n";
    }

    if (!loaded || loadedWR < TARGET_WR)
    {
        cout << "[Training...]\n";
        EvalResult base = evaluateDetailed(ai, 200, rng);
        cout << "[Initial] own-camp: " << (base.overall * 100.0) << "%\n";

        ValueAI bestAI = ai;
        double bestWR = base.overall;
        bestAI.savedWinRate = bestWR;
        saveWeights(bestAI, "ddz_best.txt", bestWR);

        const int STAGES = 8;
        const int EPISODES = 3000;
        for (int stage = 1; stage <= STAGES; stage++)
        {
            cout << "\n--- Stage " << stage << "/" << STAGES << " ---\n";
            auto t0 = chrono::steady_clock::now();

            for (int ep = 0; ep < EPISODES; ep++)
            {
                GameState g;
                shuffleDeal(g, rng);
                vector<pair<vector<double>, bool>> decisions;
                int step = 0;
                while (!g.over && step < 500)
                {
                    step++;
                    int p = g.cur;
                    bool fr = isFree(g, p);
                    auto cands = fr ? genAllMoves(g.hands[p]) : genBeats(g.hands[p], g.table);
                    if (!fr)
                        cands.push_back(Combo());
                    if (cands.empty())
                        break;
                    int idx = ai.choose(g, p, cands, rng, true, 0.10);
                    decisions.push_back({joinedFeat(g, p, cands[idx]), p == g.landlord});
                    applyMove(g, cands[idx]);
                }
                if (g.winner < 0)
                    continue;
                ai.learn(decisions, g.winner, g.landlord, rng);

                if ((ep + 1) % 1000 == 0)
                {
                    EvalResult r = evaluateDetailed(ai, 100, rng);
                    cout << "  [" << (ep + 1) << "/" << EPISODES << "] "
                         << (r.overall * 100.0) << "%"
                         << " (LL " << (r.asLandlord * 100.0)
                         << " / FM " << (r.asFarmer * 100.0) << ")\n";
                }
            }

            auto t1 = chrono::steady_clock::now();
            EvalResult r = evaluateDetailed(ai, 300, rng);
            double sec = chrono::duration<double>(t1 - t0).count();
            cout << "[Stage " << stage << "] " << sec << "s, "
                 << (r.overall * 100.0) << "% "
                 << "(LL " << (r.asLandlord * 100.0)
                 << " / FM " << (r.asFarmer * 100.0) << ")\n";

            if (r.overall > bestWR)
            {
                bestWR = r.overall;
                bestAI = ai;
                bestAI.savedWinRate = bestWR;
                saveWeights(bestAI, "ddz_best.txt", bestWR);
                cout << "  <-- new best saved\n";
            }
            else if (r.overall < bestWR - 0.02)
            {
                ai = bestAI;
                cout << "  <-- reverted\n";
            }

            if (bestWR >= TARGET_WR)
                break;
        }

        ai = bestAI;
        cout << "\nTraining done. Best: " << (bestWR * 100.0) << "%\n";
    }

    cout << "Launching GUI...\n";

    // 主循环: 玩一局 -> 可能看复盘 -> 再玩
    while (true)
    {
        g_requestReplay = false;

        launchGameGui(ai, rng);

        if (!g_requestReplay)
            break;

        showReplayViewer("ddz_replays.txt");
        // 复盘关闭后, 回到 while 顶部重开游戏窗口
    }

    saveWeights(ai, "ddz_best.txt", ai.savedWinRate);
    cv::destroyAllWindows();
    cout << "Bye!\n";
    return 0;
}