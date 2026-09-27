#include "gui_common.hpp"
#include <opencv2/opencv.hpp>

namespace ddz
{

    const char *WIN_NAME = "Dou Dizhu — OpenCV";

    cv::Point2f tablePos(int seat)
    {
        switch (seat)
        {
        case 0:
            return {WIN_W * 0.50f, WIN_H * 0.655f};
        case 1:
            return {WIN_W * 0.745f, WIN_H * 0.435f};
        case 2:
            return {WIN_W * 0.255f, WIN_H * 0.435f};
        }
        return {WIN_W * 0.5f, WIN_H * 0.5f};
    }

    cv::Point2f handOrigin(int seat)
    {
        switch (seat)
        {
        case 0:
            return {WIN_W * 0.618f, WIN_H * 0.92f};
        case 1:
            return {WIN_W * 0.94f, WIN_H * 0.50f};
        case 2:
            return {WIN_W * 0.06f, WIN_H * 0.50f};
        }
        return {WIN_W * 0.5f, WIN_H * 0.9f};
    }

    // 下面这些从旧文件原样拷过来，代码一样：
    //   buildLightMap / buildBackground / fillRoundRect / drawStar /
    //   drawSuit / drawCardFace / drawCardBack / layoutCards
    // 只需把 drawCardFace 里对 CT:: 的引用改成 ComboType:: (无)
    //
    // 新增

    cv::Mat g_lightU8;
    void buildLightMap()
    {
        cv::Mat lm(WIN_H, WIN_W, CV_32FC1);
        const float cx = WIN_W * 0.5f;
        const float cy = -WIN_H * 0.02f;
        const float maxR = sqrtf(cx * cx + (WIN_H - cy) * (WIN_H - cy));
        for (int y = 0; y < WIN_H; y++)
        {
            float *row = lm.ptr<float>(y);
            for (int x = 0; x < WIN_W; x++)
            {
                float dx = x - cx, dy = y - cy;
                float d = sqrtf(dx * dx + dy * dy);
                float t = std::min(1.0f, d / maxR);
                float b = 1.0f - 0.68f * powf(t, 1.25f);
                if (b < 0.30f)
                    b = 0.30f;
                row[x] = b;
            }
        }
        std::vector<cv::Mat> ch = {lm, lm, lm};
        cv::Mat lm3;
        cv::merge(ch, lm3);
        lm3.convertTo(g_lightU8, CV_8UC3, 255.0);
    }

    cv::Mat g_bgCache;
    void buildBackground()
    {
        cv::Mat bg(WIN_H, WIN_W, CV_8UC3, cv::Scalar(10, 8, 6));
        const float farY = WIN_H * 0.115f;
        const float farL = WIN_W * 0.075f, farR = WIN_W * 0.925f;
        const float nearL = -WIN_W * 0.06f, nearR = WIN_W * 1.06f;
        cv::Scalar cFar(28, 62, 26);
        cv::Scalar cNear(46, 108, 42);
        for (int y = (int)farY; y < WIN_H; y++)
        {
            float t = (y - farY) / (WIN_H - farY);
            float l = farL + (nearL - farL) * t;
            float r = farR + (nearR - farR) * t;
            cv::Scalar c(cFar[0] + (cNear[0] - cFar[0]) * t,
                         cFar[1] + (cNear[1] - cFar[1]) * t,
                         cFar[2] + (cNear[2] - cFar[2]) * t);
            cv::line(bg, cv::Point((int)l, y), cv::Point((int)r, y), c);
        }
        std::vector<cv::Point> edge = {
            {(int)farL, (int)farY}, {(int)farR, (int)farY}, {(int)nearR, WIN_H}, {(int)nearL, WIN_H}};
        cv::polylines(bg, std::vector<std::vector<cv::Point>>{edge}, true,
                      cv::Scalar(105, 72, 42), 7, cv::LINE_AA);
        for (int k = 1; k <= 3; k++)
            cv::ellipse(bg, cv::Point(WIN_W / 2, (int)(WIN_H * 0.60f)),
                        cv::Size((int)(WIN_W * 0.42f) - k * 14, (int)(WIN_H * 0.30f) - k * 12),
                        0, 0, 360, cv::Scalar(24, 54, 22), 1, cv::LINE_AA);
        cv::multiply(bg, g_lightU8, bg, 1.0 / 255.0);
        g_bgCache = bg;
    }

    void fillRoundRect(cv::Mat &img, cv::Rect r, int rad, const cv::Scalar &c)
    {
        if (r.width <= 0 || r.height <= 0)
            return;
        rad = std::min(rad, std::min(r.width, r.height) / 2);
        if (rad <= 0)
        {
            cv::rectangle(img, r, c, -1);
            return;
        }
        cv::rectangle(img, cv::Rect(r.x + rad, r.y, r.width - 2 * rad, r.height), c, -1);
        cv::rectangle(img, cv::Rect(r.x, r.y + rad, r.width, r.height - 2 * rad), c, -1);
        cv::circle(img, cv::Point(r.x + rad, r.y + rad), rad, c, -1, cv::LINE_AA);
        cv::circle(img, cv::Point(r.x + r.width - rad - 1, r.y + rad), rad, c, -1, cv::LINE_AA);
        cv::circle(img, cv::Point(r.x + rad, r.y + r.height - rad - 1), rad, c, -1, cv::LINE_AA);
        cv::circle(img, cv::Point(r.x + r.width - rad - 1, r.y + r.height - rad - 1), rad, c, -1, cv::LINE_AA);
    }

    static void drawStar(cv::Mat &img, cv::Point c, int r, const cv::Scalar &col)
    {
        std::vector<cv::Point> pts;
        for (int i = 0; i < 10; i++)
        {
            float a = -CV_PI / 2 + i * CV_PI / 5;
            float rr = (i % 2 == 0) ? (float)r : r * 0.45f;
            pts.push_back({(int)(c.x + rr * cosf(a)), (int)(c.y + rr * sinf(a))});
        }
        cv::fillPoly(img, std::vector<std::vector<cv::Point>>{pts}, col, cv::LINE_AA);
    }

    static void drawSuit(cv::Mat &img, int cx, int cy, int sz, int suit, const cv::Scalar &col)
    {
        switch (suit)
        {
        case 0:
        {
            std::vector<cv::Point> tri = {{cx, cy - sz}, {cx - sz, cy}, {cx + sz, cy}};
            cv::fillPoly(img, std::vector<std::vector<cv::Point>>{tri}, col, cv::LINE_AA);
            cv::circle(img, {cx - sz / 2, cy}, sz / 2, col, -1, cv::LINE_AA);
            cv::circle(img, {cx + sz / 2, cy}, sz / 2, col, -1, cv::LINE_AA);
            cv::rectangle(img, {cx - sz / 6, cy + sz / 2, std::max(2, sz / 3), sz}, col, -1);
            break;
        }
        case 1:
        {
            cv::circle(img, {cx - sz / 2, cy - sz / 3}, sz / 2, col, -1, cv::LINE_AA);
            cv::circle(img, {cx + sz / 2, cy - sz / 3}, sz / 2, col, -1, cv::LINE_AA);
            std::vector<cv::Point> tri = {{cx - sz, cy}, {cx + sz, cy}, {cx, cy + sz}};
            cv::fillPoly(img, std::vector<std::vector<cv::Point>>{tri}, col, cv::LINE_AA);
            break;
        }
        case 2:
        {
            cv::circle(img, {cx, cy - sz / 2}, sz / 2, col, -1, cv::LINE_AA);
            cv::circle(img, {cx - sz / 2, cy + sz / 3}, sz / 2, col, -1, cv::LINE_AA);
            cv::circle(img, {cx + sz / 2, cy + sz / 3}, sz / 2, col, -1, cv::LINE_AA);
            cv::rectangle(img, {cx - sz / 6, cy, std::max(2, sz / 3), sz}, col, -1);
            break;
        }
        case 3:
        {
            std::vector<cv::Point> d = {{cx, cy - sz}, {cx + sz * 3 / 4, cy}, {cx, cy + sz}, {cx - sz * 3 / 4, cy}};
            cv::fillPoly(img, std::vector<std::vector<cv::Point>>{d}, col, cv::LINE_AA);
            break;
        }
        }
    }

    static const char *RANK_STR[13] = {"3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A", "2"};

    void drawCardFace(cv::Mat &img, cv::Rect r, int card)
    {
        if (r.width <= 4 || r.height <= 6)
            return;

        fillRoundRect(img, cv::Rect(r.x + 4, r.y + 5, r.width, r.height), 7, cv::Scalar(12, 12, 12));
        fillRoundRect(img, r, 7, cv::Scalar(48, 48, 48));
        fillRoundRect(img, cv::Rect(r.x + 2, r.y + 2, r.width - 4, r.height - 4), 5, cv::Scalar(250, 249, 244));

        const float w = (float)r.width;
        if (card == 52 || card == 53)
        {
            bool big = (card == 53);
            cv::Scalar col = big ? cv::Scalar(38, 38, 210) : cv::Scalar(50, 50, 50);
            double fs = (w < 60) ? 0.42 : 0.60;
            cv::putText(img, big ? "B" : "S", {r.x + (int)(w * 0.10f), r.y + (int)(r.height * 0.24f)},
                        cv::FONT_HERSHEY_DUPLEX, fs, col, 1, cv::LINE_AA);
            int sr = (int)(std::min(r.width, r.height) * 0.24f);
            drawStar(img, {r.x + r.width / 2, r.y + r.height / 2 + r.height / 12}, sr, col);
            double fs2 = (w < 60) ? 0.32 : 0.42;
            int tw = cv::getTextSize("JOKER", cv::FONT_HERSHEY_SIMPLEX, fs2, 1, nullptr).width;
            cv::putText(img, "JOKER", {r.x + (r.width - tw) / 2, r.y + r.height - (int)(r.height * 0.10f)},
                        cv::FONT_HERSHEY_SIMPLEX, fs2, col, 1, cv::LINE_AA);
            return;
        }
        int suit = card / 13;
        int rank = card % 13;
        cv::Scalar col = (suit == 1 || suit == 3) ? cv::Scalar(38, 38, 205) : cv::Scalar(40, 40, 40);
        std::string rs = RANK_STR[rank];

        double fs = (w < 60) ? 0.48 : 0.72;
        cv::putText(img, rs, {r.x + (int)(w * 0.09f), r.y + (int)(r.height * 0.26f)},
                    cv::FONT_HERSHEY_DUPLEX, fs, col, 1, cv::LINE_AA);
        int sz = (int)(std::min(r.width, r.height) * 0.17f);
        drawSuit(img, r.x + r.width / 2, r.y + r.height * 62 / 100, sz, suit, col);
        if (w >= 60)
        {
            int s2 = (int)(sz * 0.6f);
            drawSuit(img, r.x + r.width - (int)(w * 0.22f), r.y + r.height - (int)(r.height * 0.16f), s2, suit, col);
        }
    }

    void drawCardBack(cv::Mat &img, cv::Rect r)
    {
        fillRoundRect(img, cv::Rect(r.x + 3, r.y + 4, r.width, r.height), 7, cv::Scalar(12, 12, 12));
        fillRoundRect(img, r, 7, cv::Scalar(60, 60, 60));
        fillRoundRect(img, cv::Rect(r.x + 2, r.y + 2, r.width - 4, r.height - 4), 5, cv::Scalar(138, 62, 52));
        fillRoundRect(img, cv::Rect(r.x + 5, r.y + 5, r.width - 10, r.height - 10), 3, cv::Scalar(168, 84, 66));
        int cx = r.x + r.width / 2, cy = r.y + r.height / 2;
        int s = std::min(r.width, r.height) / 5;
        std::vector<cv::Point> d = {{cx, cy - s}, {cx + s, cy}, {cx, cy + s}, {cx - s, cy}};
        cv::polylines(img, std::vector<std::vector<cv::Point>>{d}, true, cv::Scalar(210, 170, 120), 1, cv::LINE_AA);
    }

    void layoutCards(const std::vector<int> &cards, cv::Point2f center, float cw, float ch, float gap, std::vector<cv::Rect> &out)
    {
        out.clear();
        int n = (int)cards.size();
        if (n == 0)
            return;
        float total = n * cw + (n - 1) * gap;
        float sx = center.x - total / 2.0f;
        for (int i = 0; i < n; i++)
            out.push_back(cv::Rect((int)(sx + i * (cw + gap)), (int)(center.y - ch / 2), (int)cw, (int)ch));
    }

    // 复盘侧边小牌: 牌点画在顶部, 保证纵向叠放时可见
    void drawCardFaceSmall(cv::Mat &img, cv::Rect r, int card)
    {
        if (r.width <= 6 || r.height <= 8)
            return;

        // 白底 + 边
        fillRoundRect(img, r, 3, cv::Scalar(40, 40, 40));
        fillRoundRect(img, cv::Rect(r.x + 1, r.y + 1, r.width - 2, r.height - 2),
                      2, cv::Scalar(252, 252, 248));

        // 决定牌面字符
        std::string s;
        cv::Scalar col;
        if (card == 52)
        {
            s = "S";
            col = cv::Scalar(50, 50, 50);
        }
        else if (card == 53)
        {
            s = "B";
            col = cv::Scalar(38, 38, 210);
        }
        else
        {
            int suit = card / 13;
            int rank = card % 13;
            static const char *rk[] = {"3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A", "2"};
            s = rk[rank];
            col = (suit == 1 || suit == 3)
                      ? cv::Scalar(38, 38, 205)
                      : cv::Scalar(40, 40, 40);
        }

        // 字号随宽度自适应
        double fs = r.width / 42.0;
        if (fs < 0.42)
            fs = 0.42;
        if (fs > 0.80)
            fs = 0.80;
        if (s.size() >= 2)
            fs *= 0.82; // "10" 窄一点

        // ★ 关键: 文字画在顶部 30% 位置
        int baseline = 0;
        cv::Size ts = cv::getTextSize(s, cv::FONT_HERSHEY_DUPLEX, fs, 1, &baseline);
        int tx = r.x + (r.width - ts.width) / 2;
        int ty = r.y + baseline + 3; // 顶部对齐, +baseline 使其基线落在顶部
        cv::putText(img, s, {tx, ty},
                    cv::FONT_HERSHEY_DUPLEX, fs, col, 1, cv::LINE_AA);
    }

    void initGui()
    {
        buildLightMap();
        buildBackground();
    }

} // namespace ddz