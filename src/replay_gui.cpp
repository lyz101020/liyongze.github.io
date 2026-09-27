#include "replay_gui.hpp"
#include "gui_common.hpp"
#include <algorithm>
#include <opencv2/opencv.hpp>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace ddz
{

    namespace
    {

        const char *REPLAY_WIN = "Replay Viewer";

        // ---- 布局常量 ----
        constexpr int TOP_INFO_H = 50; // 顶部信息栏
        constexpr int ROW_H = 100;     // 每行手牌区高度
        constexpr int ROW_GAP = 6;
        constexpr int CARD_GAP = 3;
        constexpr int SIDE_MARGIN = 20;
        constexpr int BOTTOM_INFO_H = 180;

        struct PlaybackState
        {
            std::vector<int> hands[3];
            Combo table;
            int tablePlayer = -1;
            int step = -1;
        };

        struct ViewerState
        {
            std::vector<ReplayGame> games;
            int gameIdx = 0;
            int step = -1;
            PlaybackState state;
            cv::Point mouse{-1, -1};
            bool clicked = false;
        };

        ViewerState *g_viewer = nullptr;

        void onMouseCb(int ev, int x, int y, int, void *userdata)
        {
            auto *st = static_cast<ViewerState *>(userdata);
            if (!st)
                return;
            st->mouse = cv::Point(x, y);
            if (ev == cv::EVENT_LBUTTONDOWN)
                st->clicked = true;
        }

        // 根据事件序列重建到指定 step 的完整状态
        void rebuild(const ReplayGame &rg, int step, PlaybackState &out)
        {
            for (int i = 0; i < 3; i++)
                out.hands[i] = rg.handsInitial[i];
            out.table = Combo();
            out.tablePlayer = -1;
            out.step = step;
            for (int e = 0; e <= step && e < (int)rg.events.size(); e++)
            {
                const auto &ev = rg.events[e];
                if (!ev.pass)
                {
                    auto &h = out.hands[ev.seat];
                    for (int card : ev.cards)
                    {
                        auto it = std::find(h.begin(), h.end(), card);
                        if (it != h.end())
                            h.erase(it);
                    }
                    out.table = parseCombo(ev.cards);
                    out.tablePlayer = ev.seat;
                }
            }
        }

        // 一行手牌平铺 (自适应宽度, 完全不重叠)
        void layoutRow(const std::vector<int> &cards, int y, int height,
                       std::vector<cv::Rect> &out)
        {
            out.clear();
            int n = (int)cards.size();
            if (n == 0)
                return;

            const int availW = WIN_W - SIDE_MARGIN * 2;
            int cw;
            if (n == 1)
            {
                cw = std::min(80, availW);
            }
            else
            {
                cw = (availW - (n - 1) * CARD_GAP) / n;
                if (cw > 80)
                    cw = 80;
                if (cw < 30)
                    cw = 30;
            }
            int totalW = n * cw + (n - 1) * CARD_GAP;
            int startX = (WIN_W - totalW) / 2;
            for (int i = 0; i < n; i++)
            {
                out.push_back(cv::Rect(startX + i * (cw + CARD_GAP), y, cw, height));
            }
        }

        // 顶部信息栏
        void drawTopBar(cv::Mat &img, const ViewerState &st)
        {
            cv::rectangle(img, cv::Rect(0, 0, WIN_W, TOP_INFO_H),
                          cv::Scalar(30, 30, 30), -1);

            const ReplayGame &rg = st.games[st.gameIdx];
            char buf[256];
            snprintf(buf, sizeof(buf), "Replay %d / %d    Step %d / %d",
                     st.gameIdx + 1, (int)st.games.size(),
                     st.step, (int)rg.events.size() - 1);
            cv::putText(img, buf, {20, 34}, cv::FONT_HERSHEY_DUPLEX, 0.65,
                        cv::Scalar(100, 220, 255), 1, cv::LINE_AA);

            const char *w = (rg.winner == 0) ? "Landlord wins" : "Farmers win";
            int tw = cv::getTextSize(w, cv::FONT_HERSHEY_DUPLEX, 0.7, 1, nullptr).width;
            cv::putText(img, w, {WIN_W - 30 - tw, 34}, cv::FONT_HERSHEY_DUPLEX, 0.7,
                        cv::Scalar(255, 200, 100), 1, cv::LINE_AA);
        }

        // 一行手牌 + 座位标签 (横向完全平铺, 每张牌都能看清)
        void drawHandRow(cv::Mat &img, const std::vector<int> &cards, int seat,
                         int y, int h, const ViewerState &st)
        {
            const ReplayGame &rg = st.games[st.gameIdx];

            // 高亮: 当前 step 是该玩家出牌, 行背景变绿
            bool isActive = (st.step >= 0 && st.step < (int)rg.events.size() && rg.events[st.step].seat == seat);
            cv::Scalar bg = isActive ? cv::Scalar(45, 75, 40)
                                     : cv::Scalar(35, 35, 35);
            cv::rectangle(img, cv::Rect(0, y - 2, WIN_W, h + 4), bg, -1);

            // 座位标签 (左侧)
            const char *seatName =
                (seat == 0)   ? "You"
                : (seat == 1) ? "AI-Right"
                              : "AI-Left";
            bool isLL = (seat == rg.landlord);
            char tag[64];
            snprintf(tag, sizeof(tag), "%s%s (%d)",
                     seatName, isLL ? " [LL]" : "", (int)cards.size());

            cv::Scalar tagCol = isActive ? cv::Scalar(80, 240, 120)
                                         : cv::Scalar(200, 200, 200);
            cv::putText(img, tag, {SIDE_MARGIN, y + 18},
                        cv::FONT_HERSHEY_DUPLEX, 0.5, tagCol, 1, cv::LINE_AA);

            // 卡牌 Y 区间 (让标签和卡牌不重叠)
            int cardY = y + 22;
            int cardH = h - 24;

            std::vector<cv::Rect> rs;
            layoutRow(cards, cardY, cardH, rs);

            for (size_t i = 0; i < rs.size() && i < cards.size(); i++)
            {
                drawCardFace(img, rs[i], cards[i]);
            }

            if (cards.empty())
            {
                cv::putText(img, "(no cards left)",
                            {(WIN_W - 200) / 2, cardY + cardH / 2},
                            cv::FONT_HERSHEY_DUPLEX, 0.55,
                            cv::Scalar(120, 120, 120), 1, cv::LINE_AA);
            }
        }

        // 中央桌面
        void drawTable(cv::Mat &img, const ViewerState &st, int yTop, int yBot)
        {
            cv::rectangle(img, cv::Rect(0, yTop, WIN_W, yBot - yTop),
                          cv::Scalar(20, 55, 28), -1);

            const auto &state = st.state;

            cv::putText(img, "TABLE", {WIN_W / 2 - 42, yTop + 24},
                        cv::FONT_HERSHEY_DUPLEX, 0.55,
                        cv::Scalar(120, 200, 120), 1, cv::LINE_AA);

            if (state.table.valid())
            {
                const char *who = (state.tablePlayer == 0)   ? "You"
                                  : (state.tablePlayer == 1) ? "AI-Right"
                                                             : "AI-Left";
                char lb[192];
                snprintf(lb, sizeof(lb), "%s -> %s",
                         who, comboName(state.table).c_str());
                int tw = cv::getTextSize(lb, cv::FONT_HERSHEY_DUPLEX, 0.6, 1, nullptr).width;
                cv::putText(img, lb, {(WIN_W - tw) / 2, yTop + 54},
                            cv::FONT_HERSHEY_DUPLEX, 0.6,
                            cv::Scalar(255, 240, 120), 1, cv::LINE_AA);

                auto &cards = state.table.cards;
                int n = (int)cards.size();
                if (n > 0)
                {
                    const int cw = 62, ch = 88, gap = 6;
                    int totalW = n * cw + (n - 1) * gap;
                    int sx = (WIN_W - totalW) / 2;
                    int cy = (yTop + yBot) / 2 - ch / 2 + 20;
                    for (int i = 0; i < n; i++)
                    {
                        cv::Rect r(sx + i * (cw + gap), cy, cw, ch);
                        drawCardFace(img, r, cards[i]);
                    }
                }
            }
            else
            {
                cv::putText(img, "(initial deal - no play yet)",
                            {WIN_W / 2 - 220, (yTop + yBot) / 2 + 10},
                            cv::FONT_HERSHEY_DUPLEX, 0.65,
                            cv::Scalar(150, 150, 150), 1, cv::LINE_AA);
            }
        }

        // 底部信息栏
        void drawBottomBar(cv::Mat &img, const ViewerState &st, int yTop)
        {
            cv::rectangle(img, cv::Rect(0, yTop, WIN_W, WIN_H - yTop),
                          cv::Scalar(30, 30, 30), -1);

            const ReplayGame &rg = st.games[st.gameIdx];

            if (st.step >= 0 && st.step < (int)rg.events.size())
            {
                const auto &ev = rg.events[st.step];
                const char *who = (ev.seat == 0)   ? "You"
                                  : (ev.seat == 1) ? "AI-Right"
                                                   : "AI-Left";
                char buf[256];
                snprintf(buf, sizeof(buf), "Step %d:   %s -> %s",
                         st.step, who, ev.comboName.c_str());
                cv::putText(img, buf, {SIDE_MARGIN, yTop + 40},
                            cv::FONT_HERSHEY_DUPLEX, 0.75,
                            cv::Scalar(255, 240, 130), 1, cv::LINE_AA);

                // 本步出的牌预览
                if (!ev.cards.empty())
                {
                    int n = (int)ev.cards.size();
                    const int cw = 52, ch = 74, gap = 5;
                    int totalW = n * cw + (n - 1) * gap;
                    int sx = WIN_W - totalW - 30;
                    int cy = yTop + 20;
                    for (int i = 0; i < n; i++)
                    {
                        cv::Rect r(sx + i * (cw + gap), cy, cw, ch);
                        drawCardFace(img, r, ev.cards[i]);
                    }
                }
            }
            else
            {
                cv::putText(img, "Initial deal (Step -1)",
                            {SIDE_MARGIN, yTop + 40},
                            cv::FONT_HERSHEY_DUPLEX, 0.75,
                            cv::Scalar(200, 200, 200), 1, cv::LINE_AA);
            }

            const char *hint =
                "Left/A  prev    Right/D  next    H/Home  first    E/End  last    N/P  next/prev game    ESC  exit";
            cv::putText(img, hint, {SIDE_MARGIN, WIN_H - 20},
                        cv::FONT_HERSHEY_DUPLEX, 0.5,
                        cv::Scalar(150, 150, 150), 1, cv::LINE_AA);
        }

        void renderViewer(cv::Mat &img, ViewerState &st)
        {
            img = cv::Mat(WIN_H, WIN_W, CV_8UC3, cv::Scalar(15, 15, 15));

            if (st.games.empty())
            {
                cv::putText(img, "No replay data.",
                            {WIN_W / 2 - 150, WIN_H / 2},
                            cv::FONT_HERSHEY_DUPLEX, 1.2,
                            cv::Scalar(220, 220, 220), 2);
                return;
            }

            // 布局 Y 坐标
            int y_aiLeft_top = TOP_INFO_H;
            int y_aiLeft_bot = y_aiLeft_top + ROW_H;
            int y_desk_top = y_aiLeft_bot + ROW_GAP;
            int y_desk_bot = WIN_H - BOTTOM_INFO_H - ROW_H * 2 - ROW_GAP * 2;
            int y_aiRight_top = y_desk_bot + ROW_GAP;
            int y_aiRight_bot = y_aiRight_top + ROW_H;
            int y_you_top = y_aiRight_bot + ROW_GAP;
            int y_you_bot = y_you_top + ROW_H;
            int y_bottom_top = y_you_bot + ROW_GAP;

            // 绘制顺序: 从上到下
            drawTopBar(img, st);
            drawHandRow(img, st.state.hands[2], 2, y_aiLeft_top, ROW_H, st);
            drawTable(img, st, y_desk_top, y_desk_bot);
            drawHandRow(img, st.state.hands[1], 1, y_aiRight_top, ROW_H, st);
            drawHandRow(img, st.state.hands[0], 0, y_you_top, ROW_H, st);
            drawBottomBar(img, st, y_bottom_top);
        }

    } // anonymous namespace

    // ============================================================
    // 公开接口
    // ============================================================
    void showReplayViewer(const std::string &path)
    {
        auto games = loadReplays(path);
        if (games.empty())
        {
            std::cout << "[Replay] no data in " << path << "\n";
            return;
        }

        ViewerState st;
        st.games = std::move(games);
        st.gameIdx = (int)st.games.size() - 1;
        st.step = -1;
        rebuild(st.games[st.gameIdx], st.step, st.state);

        g_viewer = &st;

        cv::namedWindow(REPLAY_WIN, cv::WINDOW_NORMAL);
        cv::resizeWindow(REPLAY_WIN, 1280, 800);
        cv::setMouseCallback(REPLAY_WIN, onMouseCb, &st);

        cv::Mat img;
        while (true)
        {
            renderViewer(img, st);
            cv::imshow(REPLAY_WIN, img);
            int key = cv::waitKey(30);

            if (key == 27)
                break;

            const ReplayGame &rg = st.games[st.gameIdx];
            int nEvents = (int)rg.events.size();

            if (key == 81 || key == 'a' || key == 'A')
            {
                if (st.step >= -1)
                {
                    st.step--;
                    rebuild(rg, st.step, st.state);
                }
            }
            else if (key == 83 || key == 'd' || key == 'D')
            {
                if (st.step < nEvents - 1)
                {
                    st.step++;
                    rebuild(rg, st.step, st.state);
                }
            }
            else if (key == 'n' || key == 'N')
            {
                if (st.gameIdx + 1 < (int)st.games.size())
                {
                    st.gameIdx++;
                    st.step = -1;
                    rebuild(st.games[st.gameIdx], st.step, st.state);
                }
            }
            else if (key == 'p' || key == 'P')
            {
                if (st.gameIdx > 0)
                {
                    st.gameIdx--;
                    st.step = -1;
                    rebuild(st.games[st.gameIdx], st.step, st.state);
                }
            }
            else if (key == 80 || key == 'h' || key == 'H')
            {
                st.step = -1;
                rebuild(rg, st.step, st.state);
            }
            else if (key == 87 || key == 'e' || key == 'E')
            {
                st.step = nEvents - 1;
                rebuild(rg, st.step, st.state);
            }
        }

        cv::destroyWindow(REPLAY_WIN);
        g_viewer = nullptr;
    }

} // namespace ddz