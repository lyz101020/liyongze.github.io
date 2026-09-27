#pragma once
#include "replay.hpp"
#include <string>

namespace ddz
{

    // 打开复盘查看窗口, 加载 path 中所有对局
    // 按 ESC 返回
    void showReplayViewer(const std::string &path);

} // namespace ddz