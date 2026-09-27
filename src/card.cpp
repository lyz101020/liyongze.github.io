#include "card.hpp"
#include <algorithm>

namespace ddz
{

    std::string cardName(int card)
    {
        static const char *rk[] = {"3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A", "2"};
        static const char *st[] = {"S", "H", "C", "D"};
        if (card == 52)
            return "BJ";
        if (card == 53)
            return "RJ";
        return std::string(st[card / 13]) + rk[card % 13];
    }

    const char *pointName(int p)
    {
        static const char *names[] = {"", "", "", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A", "2", "BJ", "RJ"};
        if (p < 3 || p > 17)
            return "?";
        return names[p];
    }

    void sortHand(std::vector<int> &h)
    {
        std::sort(h.begin(), h.end(), [](int a, int b)
                  {
        int pa = pointOf(a), pb = pointOf(b);
        if (pa != pb) return pa < pb;
        return a < b; });
    }

} // namespace ddz