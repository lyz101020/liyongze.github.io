#include "game_gui.hpp"
#include "gui_common.hpp"
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

using namespace std;

namespace ddz
{

    // 由 main.cpp 定义, 通知主循环打开回放查看器
    extern bool g_requestReplay;

    // g_bgCache 由 gui_common.cpp 定义
    extern cv::Mat g_bgCache;

    namespace
    {

        // ---------------- UI 状态 ----------------
        struct UIState
        {
            cv::Point mouse{-1, -1};
            bool clicked = false;

            vector<int> selected;
            vector<cv::Rect> handRects;

            cv::Rect btnPlay, btnPass;
            bool playEnabled = false, passEnabled = false;

            vector<pair<string, cv::Scalar>> log;
            vector<int> tableCards[3];

            void addLog(const string &s, const cv::Scalar &c = cv::Scalar(215, 215, 215))
            {
                log.push_back({s, c});
                if (log.size() > 60)
                    log.erase(log.begin());
            }
        };

        UIState *g_uiPtr = nullptr;
        int g_humanWins = 0;
        int g_humanTotal = 0;

        void handleMouse(int ev, int x, int y, int /*flags*/, void * /*userdata*/)
        {
            if (!g_uiPtr)
                return;
            g_uiPtr->mouse = cv::Point(x, y);
            if (ev == cv::EVENT_LBUTTONDOWN)
                g_uiPtr->clicked = true;
        }

        // ---------------- 发牌 ----------------
        void dealNewGame(GameState &g, mt19937 &rng, vector<int> &bottomOut)
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
            double bestS = -1;
            for (int i = 0; i < 3; i++)
            {
                double s = handStrength(g.hands[i]);
                if (s > bestS)
                {
                    bestS = s;
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

            bottomOut = bottom;
        }

        // ---------------- 渲染一帧 ----------------
        void renderGame(cv::Mat &img, const GameState &g, UIState &ui, int humanSeat)
        {
            g_bgCache.copyTo(img);

            // 1. 桌上已出的牌
            for (int s = 0; s < 3; s++)
            {
                if (ui.tableCards[s].empty())
                    continue;
                cv::Point2f c = tablePos(s);
                float cw = 54, ch = 76, gap = 4;
                vector<cv::Rect> rs;
                layoutCards(ui.tableCards[s], c, cw, ch, gap, rs);
                for (size_t i = 0; i < rs.size(); i++)
                    drawCardFace(img, rs[i], ui.tableCards[s][i]);
            }

            // 2. 其他玩家手牌 (背面堆叠)
            for (int s = 0; s < 3; s++)
            {
                if (s == humanSeat)
                    continue;
                int n = (int)g.hands[s].size();
                if (n == 0)
                    continue;
                float cw = 34, ch = 48;
                bool left = (s == 2);
                float bx = left ? 26.0f : (float)(WIN_W - 26 - cw);
                float step = min(9.0f, 210.0f / max(1, n - 1));
                float totalH = ch + (n - 1) * step;
                float by = 430 - totalH / 2;
                for (int i = 0; i < n; i++)
                {
                    cv::Rect r((int)bx, (int)(by + i * step), (int)cw, (int)ch);
                    drawCardBack(img, r);
                }
            }

            // 3. 人类手牌
            {
                const auto &hand = g.hands[humanSeat];
                int n = (int)hand.size();
                ui.handRects.clear();
                if (n > 0)
                {
                    float cw = 78, ch = 110;
                    const float availW = 1070.0f;
                    float stride = (n <= 1) ? cw : min(cw, (availW - cw) / (n - 1));
                    float totalW = cw + (n - 1) * stride;
                    float startX = 893.0f - totalW / 2.0f;
                    float baseY = 880.0f - ch;

                    for (int i = 0; i < n; i++)
                    {
                        bool sel = false;
                        for (int k : ui.selected)
                            if (k == i)
                            {
                                sel = true;
                                break;
                            }
                        cv::Rect hit((int)(startX + i * stride), (int)baseY, (int)cw, (int)ch);
                        ui.handRects.push_back(hit);
                        float y = baseY - (sel ? 26.0f : 0.0f);
                        cv::Rect r(hit.x, (int)y, hit.width, hit.height);
                        drawCardFace(img, r, hand[i]);
                        if (sel)
                            cv::rectangle(img, cv::Rect(r.x - 1, r.y - 1, r.width + 2, r.height + 2),
                                          cv::Scalar(60, 220, 255), 2, cv::LINE_AA);
                    }
                }
            }

            // 4. 玩家信息
            auto drawPlayerTag = [&](int seat)
            {
                bool isLL = (seat == g.landlord);
                string name = (seat == humanSeat) ? "You" : ("AI-" + to_string(seat));
                if (isLL)
                    name += " [Landlord]";
                int cardN = (int)g.hands[seat].size();
                string cnt = "Cards: " + to_string(cardN);
                cv::Scalar col = (seat == g.cur && !g.over) ? cv::Scalar(50, 230, 255)
                                                            : cv::Scalar(220, 220, 220);
                int base = (seat == 1) ? (WIN_W - 190) : (seat == 2 ? 70 : 780);
                cv::putText(img, name, {base, (seat == 0 ? 128 : 152)},
                            cv::FONT_HERSHEY_DUPLEX, 0.58, col, 1, cv::LINE_AA);
                cv::putText(img, cnt, {base, (seat == 0 ? 148 : 172)},
                            cv::FONT_HERSHEY_DUPLEX, 0.52, cv::Scalar(200, 200, 200), 1, cv::LINE_AA);
                if (isLL)
                    cv::putText(img, "LANDLORD", {base, (seat == 0 ? 168 : 192)},
                                cv::FONT_HERSHEY_DUPLEX, 0.44, cv::Scalar(60, 200, 245), 1, cv::LINE_AA);
            };
            drawPlayerTag(0);
            drawPlayerTag(1);
            drawPlayerTag(2);

            // 5. 按钮
            auto drawButton = [&](cv::Rect r, const char *label, bool enabled, bool hover)
            {
                cv::Scalar base = enabled ? (hover ? cv::Scalar(90, 170, 235)
                                                   : cv::Scalar(60, 120, 180))
                                          : cv::Scalar(75, 75, 75);
                fillRoundRect(img, r, 8, base);
                fillRoundRect(img, cv::Rect(r.x + 2, r.y + 2, r.width - 4, r.height - 4), 6,
                              enabled ? (hover ? cv::Scalar(115, 195, 250)
                                               : cv::Scalar(78, 145, 205))
                                      : cv::Scalar(92, 92, 92));
                int tw = cv::getTextSize(label, cv::FONT_HERSHEY_DUPLEX, 0.62, 1, nullptr).width;
                cv::putText(img, label, {r.x + (r.width - tw) / 2, r.y + r.height / 2 + 8},
                            cv::FONT_HERSHEY_DUPLEX, 0.62,
                            enabled ? cv::Scalar(255, 255, 255) : cv::Scalar(140, 140, 140),
                            1, cv::LINE_AA);
            };

            ui.btnPlay = cv::Rect(1152, 686, 108, 48);
            ui.btnPass = cv::Rect(1276, 686, 108, 48);
            drawButton(ui.btnPlay, "PLAY", ui.playEnabled, ui.btnPlay.contains(ui.mouse));
            drawButton(ui.btnPass, "PASS", ui.passEnabled, ui.btnPass.contains(ui.mouse));

            // 6. 聊天框
            {
                cv::Rect panel(14, 596, 322, 292);
                cv::Mat roi = img(panel);
                cv::Mat dark(roi.size(), CV_8UC3, cv::Scalar(16, 14, 12));
                cv::addWeighted(dark, 0.78, roi, 0.22, 0, roi);
                cv::rectangle(img, panel, cv::Scalar(95, 88, 74), 1, cv::LINE_AA);
                cv::putText(img, "TABLE LOG", {panel.x + 12, panel.y + 24},
                            cv::FONT_HERSHEY_DUPLEX, 0.48, cv::Scalar(70, 200, 240), 1, cv::LINE_AA);
                cv::line(img, {panel.x + 12, panel.y + 34},
                         {panel.x + panel.width - 12, panel.y + 34},
                         cv::Scalar(80, 74, 62), 1, cv::LINE_AA);
                int maxLines = 11;
                int start = max(0, (int)ui.log.size() - maxLines);
                int y = panel.y + 58;
                for (int i = start; i < (int)ui.log.size(); i++)
                {
                    cv::putText(img, ui.log[i].first, {panel.x + 12, y},
                                cv::FONT_HERSHEY_SIMPLEX, 0.42, ui.log[i].second, 1, cv::LINE_AA);
                    y += 21;
                }
            }

            // 7. 顶部横幅
            if (!g.over)
            {
                string turnTxt = (g.cur == humanSeat) ? "YOUR TURN"
                                                      : ("AI-" + to_string(g.cur) + " THINKING...");
                cv::Scalar col = (g.cur == humanSeat) ? cv::Scalar(90, 240, 120)
                                                      : cv::Scalar(70, 210, 250);
                int tw = cv::getTextSize(turnTxt, cv::FONT_HERSHEY_DUPLEX, 0.78, 2, nullptr).width;
                cv::putText(img, turnTxt, {(WIN_W - tw) / 2, 52},
                            cv::FONT_HERSHEY_DUPLEX, 0.78, col, 2, cv::LINE_AA);
            }

            // 8. 需要压的牌型
            if (!g.over && g.table.valid() && !isFree(g, g.cur))
            {
                string needTxt = string("Must beat: ") + comboName(g.table);
                cv::putText(img, needTxt, {14, 92}, cv::FONT_HERSHEY_SIMPLEX, 0.55,
                            cv::Scalar(240, 200, 120), 1, cv::LINE_AA);
            }
        }

        // ---------------- 飞行动画 ----------------
        void animateFly(cv::Mat &img, const GameState &g, UIState &ui, int seat,
                        const vector<int> &cards, int humanSeat)
        {
            if (cards.empty())
                return;
            cv::Point2f from = handOrigin(seat);
            cv::Point2f to = tablePos(seat);
            float cw = 54, ch = 76, gap = 4;
            vector<cv::Rect> dst;
            layoutCards(cards, to, cw, ch, gap, dst);

            auto t0 = chrono::steady_clock::now();
            const float DUR = 0.38f;
            while (true)
            {
                float el = chrono::duration<float>(chrono::steady_clock::now() - t0).count();
                float t = min(1.0f, el / DUR);
                float e = 1.0f - powf(1.0f - t, 3.0f);
                renderGame(img, g, ui, humanSeat);
                for (size_t i = 0; i < cards.size(); i++)
                {
                    cv::Point2f target(dst[i].x + dst[i].width / 2.0f,
                                       dst[i].y + dst[i].height / 2.0f);
                    cv::Point2f p = from + (target - from) * e;
                    cv::Rect r((int)(p.x - cw / 2), (int)(p.y - ch / 2), (int)cw, (int)ch);
                    drawCardFace(img, r, cards[i]);
                }
                cv::imshow(WIN_NAME, img);
                cv::waitKey(1);
                if (t >= 1.0f)
                    break;
            }
        }

    } // anonymous namespace

    // ---------------- onMouse 暴露给外部 ----------------
    void onMouse(int ev, int x, int y, int flags, void *userdata)
    {
        handleMouse(ev, x, y, flags, userdata);
    }

    // ---------------- 一局游戏 ----------------
    bool playHumanGame(ValueAI &ai, mt19937 &rng, ReplayRecorder &rec)
    {
        GameState g;
        vector<int> bottom;
        dealNewGame(g, rng, bottom);

        UIState ui;
        g_uiPtr = &ui;

        rec.beginGame(g, bottom);

        ui.addLog("=== New Game ===", cv::Scalar(90, 220, 255));
        ui.addLog(string("You are ") + (g.landlord == 0 ? "LANDLORD" : "FARMER"),
                  cv::Scalar(90, 220, 255));
        ui.addLog("Landlord: Player " + to_string(g.landlord),
                  cv::Scalar(230, 200, 130));

        cv::Mat img;
        vector<pair<vector<double>, bool>> aiDec;
        const int humanSeat = 0;

        while (!g.over)
        {
            int p = g.cur;
            bool fr = isFree(g, p);

            if (p == humanSeat)
            {
                ui.selected.clear();
                ui.playEnabled = true;
                ui.passEnabled = !fr;

                Combo chosen;
                bool decided = false;

                while (!decided)
                {
                    renderGame(img, g, ui, humanSeat);
                    cv::imshow(WIN_NAME, img);
                    int key = cv::waitKey(16);
                    if (key == 27)
                    {
                        g_uiPtr = nullptr;
                        return false;
                    }

                    if (ui.clicked)
                    {
                        ui.clicked = false;
                        if (ui.btnPlay.contains(ui.mouse))
                        {
                            vector<int> cd;
                            for (int idx : ui.selected)
                                if (idx >= 0 && idx < (int)g.hands[0].size())
                                    cd.push_back(g.hands[0][idx]);
                            Combo c = parseCombo(cd);
                            if (!c.valid())
                                ui.addLog("Invalid combo!", cv::Scalar(90, 90, 250));
                            else if (!fr && !canBeat(c, g.table))
                                ui.addLog("Does not beat last play!", cv::Scalar(90, 90, 250));
                            else
                            {
                                chosen = c;
                                decided = true;
                            }
                            continue;
                        }
                        if (ui.btnPass.contains(ui.mouse))
                        {
                            if (fr)
                                ui.addLog("Cannot pass when leading!", cv::Scalar(90, 90, 250));
                            else
                            {
                                chosen = Combo();
                                decided = true;
                            }
                            continue;
                        }
                        for (int i = (int)ui.handRects.size() - 1; i >= 0; i--)
                        {
                            if (ui.handRects[i].contains(ui.mouse))
                            {
                                auto it = find(ui.selected.begin(), ui.selected.end(), i);
                                if (it != ui.selected.end())
                                    ui.selected.erase(it);
                                else
                                    ui.selected.push_back(i);
                                break;
                            }
                        }
                    }
                }

                rec.logMove(humanSeat, chosen);

                if (chosen.valid())
                {
                    for (int c : chosen.cards)
                    {
                        auto &h = g.hands[0];
                        auto it = find(h.begin(), h.end(), c);
                        if (it != h.end())
                            h.erase(it);
                    }
                }
                ui.tableCards[0].clear();
                if (chosen.valid())
                {
                    animateFly(img, g, ui, 0, chosen.cards, humanSeat);
                    ui.tableCards[0] = chosen.cards;
                    ui.addLog("You -> " + comboName(chosen), cv::Scalar(80, 230, 130));
                }
                else
                {
                    ui.addLog("You -> PASS", cv::Scalar(190, 190, 190));
                }
                applyMove(g, chosen);
                ui.selected.clear();
            }
            else
            {
                // AI 思考停顿
                {
                    auto t0 = chrono::steady_clock::now();
                    while (chrono::duration<float>(chrono::steady_clock::now() - t0).count() < 0.55f)
                    {
                        renderGame(img, g, ui, humanSeat);
                        cv::imshow(WIN_NAME, img);
                        int key = cv::waitKey(15);
                        if (key == 27)
                        {
                            g_uiPtr = nullptr;
                            return false;
                        }
                    }
                }

                vector<Combo> cands = fr ? genAllMoves(g.hands[p])
                                         : genBeats(g.hands[p], g.table);
                if (!fr)
                    cands.push_back(Combo());

                Combo chosen;
                if (!cands.empty())
                {
                    int idx = ai.choose(g, p, cands, rng, false);
                    chosen = cands[idx];
                    aiDec.push_back({joinedFeat(g, p, chosen), p == g.landlord});
                }

                rec.logMove(p, chosen);

                if (chosen.valid())
                {
                    for (int c : chosen.cards)
                    {
                        auto &h = g.hands[p];
                        auto it = find(h.begin(), h.end(), c);
                        if (it != h.end())
                            h.erase(it);
                    }
                }
                ui.tableCards[p].clear();
                if (chosen.valid())
                {
                    animateFly(img, g, ui, p, chosen.cards, humanSeat);
                    ui.tableCards[p] = chosen.cards;
                    ui.addLog("AI-" + to_string(p) + " -> " + comboName(chosen),
                              cv::Scalar(200, 200, 200));
                }
                else
                {
                    ui.addLog("AI-" + to_string(p) + " -> PASS", cv::Scalar(150, 150, 150));
                }
                applyMove(g, chosen);
            }
        }

        // 结束
        bool humanWin = (g.landlord == 0) ? (g.winner == 0) : (g.winner == 1);
        g_humanTotal++;
        if (humanWin)
            g_humanWins++;

        if (!aiDec.empty())
            ai.learn(aiDec, g.winner, g.landlord, rng);

        rec.endGame(g.winner);
        rec.appendToFile("ddz_replays.txt");

        // 结束画面
        renderGame(img, g, ui, humanSeat);
        {
            cv::Mat ov = img.clone();
            cv::rectangle(ov, cv::Rect(0, 0, WIN_W, WIN_H), cv::Scalar(0, 0, 0), -1);
            cv::addWeighted(ov, 0.55, img, 0.45, 0, img);

            string res = humanWin ? "YOU WIN!" : "YOU LOSE";
            cv::Scalar col = humanWin ? cv::Scalar(80, 240, 120)
                                      : cv::Scalar(80, 80, 240);
            int tw = cv::getTextSize(res, cv::FONT_HERSHEY_DUPLEX, 2.2, 4, nullptr).width;
            cv::putText(img, res, {(WIN_W - tw) / 2, WIN_H / 2 - 20},
                        cv::FONT_HERSHEY_DUPLEX, 2.2, col, 4, cv::LINE_AA);

            string sub = (g.winner == 0 ? "Landlord wins" : "Farmers win");
            int tw2 = cv::getTextSize(sub, cv::FONT_HERSHEY_DUPLEX, 0.9, 2, nullptr).width;
            cv::putText(img, sub, {(WIN_W - tw2) / 2, WIN_H / 2 + 50},
                        cv::FONT_HERSHEY_DUPLEX, 0.9, cv::Scalar(230, 230, 230), 2, cv::LINE_AA);

            char buf[128];
            snprintf(buf, sizeof(buf), "Human win rate: %d / %d = %.1f%%",
                     g_humanWins, g_humanTotal,
                     100.0 * g_humanWins / max(1, g_humanTotal));
            int tw3 = cv::getTextSize(buf, cv::FONT_HERSHEY_DUPLEX, 0.62, 1, nullptr).width;
            cv::putText(img, buf, {(WIN_W - tw3) / 2, WIN_H / 2 + 105},
                        cv::FONT_HERSHEY_DUPLEX, 0.62, cv::Scalar(180, 220, 255), 1, cv::LINE_AA);

            string hint = "Y = next game   R = replay   N/ESC = quit";
            int tw4 = cv::getTextSize(hint, cv::FONT_HERSHEY_DUPLEX, 0.55, 1, nullptr).width;
            cv::putText(img, hint, {(WIN_W - tw4) / 2, WIN_H / 2 + 165},
                        cv::FONT_HERSHEY_DUPLEX, 0.55, cv::Scalar(170, 170, 170), 1, cv::LINE_AA);
        }
        cv::imshow(WIN_NAME, img);

        while (true)
        {
            int key = cv::waitKey(0);
            if (key == 'y' || key == 'Y')
            {
                g_uiPtr = nullptr;
                return true;
            }
            if (key == 'r' || key == 'R')
            {
                // 通知 main 打开回放查看器
                g_uiPtr = nullptr;
                g_requestReplay = true;
                return true; // 让 main 处理回放后再开新游戏
            }
            if (key == 'n' || key == 'N' || key == 27)
            {
                g_uiPtr = nullptr;
                return false;
            }
        }
    }

    // ---------------- GUI 主循环 ----------------
    void launchGameGui(ValueAI &ai, mt19937 &rng)
    {
        initGui();
        cv::namedWindow(WIN_NAME, cv::WINDOW_NORMAL);
        cv::resizeWindow(WIN_NAME, 1280, 800);
        cv::setMouseCallback(WIN_NAME, onMouse, nullptr);

        ReplayRecorder rec;
        while (true)
        {
            g_requestReplay = false;
            bool again = playHumanGame(ai, rng, rec);

            if (g_requestReplay)
            {
                // 由 main 处理复盘查看
                saveWeights(ai, "ddz_best.txt", ai.savedWinRate);
                cv::destroyWindow(WIN_NAME);
                return;
            }
            if (!again)
                break;
        }
        saveWeights(ai, "ddz_best.txt", ai.savedWinRate);
        cv::destroyAllWindows();
        cout << "\nSaved ddz_best.txt. Bye!\n";
    }

} // namespace ddz