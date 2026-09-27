#pragma once
#include "card.hpp"
#include <vector>

namespace ddz
{

    enum class ComboType
    {
        Invalid,
        Single,
        Pair,
        Triple,
        TripleSingle,
        TriplePair,
        Straight,
        PairStraight,
        Plane,
        PlaneSingle,
        PlanePair,
        FourTwoSingle,
        FourTwoPair,
        Bomb,
        Rocket
    };

    struct Combo
    {
        ComboType type = ComboType::Invalid;
        int mainRank = 0;
        int len = 1;
        std::vector<int> cards;
        bool valid() const { return type != ComboType::Invalid; }
    };

    const char *ctNameEN(ComboType t);
    std::string comboName(const Combo &c);
    Combo parseCombo(const std::vector<int> &cards);
    bool canBeat(const Combo &a, const Combo &b);

    std::vector<Combo> genAllMoves(const std::vector<int> &hand);
    std::vector<Combo> genBeats(const std::vector<int> &hand, const Combo &last);

} // namespace ddz