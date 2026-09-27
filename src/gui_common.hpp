#pragma once
#include "card.hpp"
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

namespace ddz
{

    constexpr int WIN_W = 1440;
    constexpr int WIN_H = 900;
    extern const char *WIN_NAME;

    cv::Point2f tablePos(int seat);
    cv::Point2f handOrigin(int seat);

    void initGui(); // 进程内调用一次

    void fillRoundRect(cv::Mat &img, cv::Rect r, int rad, const cv::Scalar &c);
    void drawCardFace(cv::Mat &img, cv::Rect r, int card);
    void drawCardBack(cv::Mat &img, cv::Rect r);
    void drawCardFaceSmall(cv::Mat &img, cv::Rect r, int card); // 复盘用

    void layoutCards(const std::vector<int> &cards, cv::Point2f center,
                     float cw, float ch, float gap, std::vector<cv::Rect> &out);

} // namespace ddz