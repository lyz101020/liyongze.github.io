#include "replay.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

namespace ddz
{

    void ReplayRecorder::beginGame(const GameState &g, const std::vector<int> &bottom)
    {
        game_ = ReplayGame();
        game_.landlord = g.landlord;
        game_.bottom = bottom;
        for (int i = 0; i < 3; i++)
            game_.handsInitial[i] = g.hands[i];
    }

    void ReplayRecorder::logMove(int seat, const Combo &c)
    {
        ReplayEvent e;
        e.seat = seat;
        e.pass = !c.valid();
        e.cards = c.cards;
        e.comboName = comboName(c);
        game_.events.push_back(std::move(e));
    }

    void ReplayRecorder::endGame(int winner)
    {
        game_.winner = winner;
    }

    void ReplayRecorder::appendToFile(const std::string &path) const
    {
        std::ofstream f(path, std::ios::app);
        if (!f)
            return;

        // 格式:
        //   G <landlord> <winner>
        //   I <seat> <card1> <card2> ...    (3 行)
        //   B <c1> <c2> <c3>
        //   E <seat> <pass> <n> <c1> ... <cn>    (重复)
        f << "G " << game_.landlord << " " << game_.winner << "\n";
        for (int s = 0; s < 3; s++)
        {
            f << "I " << s;
            for (int c : game_.handsInitial[s])
                f << " " << c;
            f << "\n";
        }
        f << "B";
        for (int c : game_.bottom)
            f << " " << c;
        f << "\n";
        for (const auto &e : game_.events)
        {
            f << "E " << e.seat << " " << (e.pass ? 1 : 0)
              << " " << (int)e.cards.size();
            for (int c : e.cards)
                f << " " << c;
            f << "\n";
        }
        f << "---\n";
    }

    std::vector<ReplayGame> loadReplays(const std::string &path)
    {
        std::ifstream f(path);
        if (!f)
            return {};

        std::vector<ReplayGame> out;
        ReplayGame cur;
        bool inGame = false;
        std::string line;

        auto finalize = [&]()
        {
            if (inGame)
            {
                out.push_back(std::move(cur));
                cur = ReplayGame();
                inGame = false;
            }
        };

        while (std::getline(f, line))
        {
            if (line.empty())
                continue;
            if (line == "---")
            {
                finalize();
                continue;
            }

            std::istringstream ss(line);
            char tag;
            ss >> tag;
            if (tag == 'G')
            {
                finalize();
                inGame = true;
                ss >> cur.landlord >> cur.winner;
            }
            else if (tag == 'I')
            {
                int seat;
                ss >> seat;
                int c;
                while (ss >> c)
                    cur.handsInitial[seat].push_back(c);
            }
            else if (tag == 'B')
            {
                int c;
                while (ss >> c)
                    cur.bottom.push_back(c);
            }
            else if (tag == 'E')
            {
                ReplayEvent e;
                int passFlag, n;
                ss >> e.seat >> passFlag >> n;
                e.pass = (passFlag != 0);
                for (int i = 0; i < n; i++)
                {
                    int c;
                    ss >> c;
                    e.cards.push_back(c);
                }
                e.comboName = comboName(parseCombo(e.cards));
                if (e.pass)
                    e.comboName = "Pass";
                cur.events.push_back(std::move(e));
            }
        }
        finalize();
        return out;
    }

} // namespace ddz