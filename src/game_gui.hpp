#pragma once
#include "value_ai.hpp"
#include "replay.hpp"
#include <random>

namespace ddz
{

    // 跑一局人机对战 (GUI 事件循环内置)
    // 返回 false 表示用户按 N/ESC 退出
    bool playHumanGame(ValueAI &ai, std::mt19937 &rng, ReplayRecorder &rec);

    // GUI 主入口: 循环调用 playHumanGame, 并在结束后询问是否看复盘
    void launchGameGui(ValueAI &ai, std::mt19937 &rng);

} // namespace ddz