#pragma once
#include <string>
#include <vector>

namespace ddz
{

    // 牌编号约定：0~51 是 3S 3H 3C 3D ... 2S 2H 2C 2D，52 小王，53 大王
    inline int pointOf(int card)
    {
        if (card >= 52)
            return card - 36;
        return card % 13 + 3;
    }

    std::string cardName(int card);
    const char *pointName(int p);
    void sortHand(std::vector<int> &h);

} // namespace ddz