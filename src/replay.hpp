#pragma once
#include "game_state.hpp"
#include <string>
#include <vector>

namespace ddz
{

    // 一次出牌或过牌的记录
    struct ReplayEvent
    {
        int seat = 0;
        bool pass = false;
        std::vector<int> cards; // pass 时为空
        std::string comboName;  // "Pair 8" 或 "Pass"
    };

    // 一局完整记录
    struct ReplayGame
    {
        int landlord = 0;
        int winner = -1;                  // 0=地主赢, 1=农民赢
        std::vector<int> handsInitial[3]; // 起始手牌 (地主已含底牌)
        std::vector<int> bottom;          // 底牌
        std::vector<ReplayEvent> events;
    };

    // 对局中记录器
    class ReplayRecorder
    {
    public:
        void beginGame(const GameState &g, const std::vector<int> &bottom);
        void logMove(int seat, const Combo &c);
        void endGame(int winner);
        const ReplayGame &current() const { return game_; }

        // 追加到文件 (每行一局)
        void appendToFile(const std::string &path) const;

    private:
        ReplayGame game_;
    };

    // 从文件加载全部复盘
    std::vector<ReplayGame> loadReplays(const std::string &path);

} // namespace ddz