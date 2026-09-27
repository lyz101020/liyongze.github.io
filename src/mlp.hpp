#pragma once
#include "features.hpp"
#include <random>
#include <vector>

namespace ddz
{

    constexpr int HID_DIM = 128;

    struct MLP
    {
        int inD = INPUT_DIM;
        int hidD = HID_DIM;
        std::vector<std::vector<double>> W1;
        std::vector<double> b1, W2;
        double b2 = 0.0;

        std::vector<double> last_x, last_z1, last_h;
        double last_y = 0.0;

        MLP();
        void init(std::mt19937 &rng);
        double forward(const std::vector<double> &x);
        double backward(double target, double lr);
    };

    inline double sigmoid(double x)
    {
        if (x > 30)
            return 1.0;
        if (x < -30)
            return 0.0;
        return 1.0 / (1.0 + std::exp(-x));
    }

} // namespace ddz