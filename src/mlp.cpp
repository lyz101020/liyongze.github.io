#include "features.hpp"
#include "game_state.hpp"
#include "mlp.hpp"
#include <vector>
#include <random>

namespace ddz
{
    MLP::MLP()
    {
        W1.assign(hidD, std::vector<double>(inD, 0.0));
        b1.assign(hidD, 0.0);
        W2.assign(hidD, 0.0);
    }

    void MLP::init(std::mt19937 &rng)
    {
        double s1 = sqrt(2.0 / inD), s2 = sqrt(2.0 / hidD);
        std::normal_distribution<double> n1(0, s1), n2(0, s2);
        for (int i = 0; i < hidD; i++)
            for (int j = 0; j < inD; j++)
                W1[i][j] = n1(rng);
        for (int i = 0; i < hidD; i++)
            b1[i] = 0.0;
        for (int i = 0; i < hidD; i++)
            W2[i] = n2(rng);
        b2 = 0.0;
    }

    double MLP::forward(const std::vector<double> &x)
    {
        last_x = x;
        last_z1.resize(hidD);
        last_h.resize(hidD);
        for (int i = 0; i < hidD; i++)
        {
            double s = b1[i];
            const auto &row = W1[i];
            for (int j = 0; j < inD; j++)
                s += row[j] * x[j];
            last_z1[i] = s;
            last_h[i] = (s > 0) ? s : 0.0;
        }
        double s = b2;
        for (int i = 0; i < hidD; i++)
            s += W2[i] * last_h[i];
        last_y = sigmoid(s);
        return last_y;
    }

    double MLP::backward(double target, double lr)
    {
        double err = last_y - target;
        double dL_dz2 = err * last_y * (1.0 - last_y);

        std::vector<double> dL_dh(hidD);
        for (int i = 0; i < hidD; i++)
            dL_dh[i] = dL_dz2 * W2[i];

        for (int i = 0; i < hidD; i++)
            W2[i] -= lr * dL_dz2 * last_h[i];
        b2 -= lr * dL_dz2;

        for (int i = 0; i < hidD; i++)
        {
            double dL_dz1 = (last_z1[i] > 0) ? dL_dh[i] : 0.0;
            auto &row = W1[i];
            for (int j = 0; j < inD; j++)
                row[j] -= lr * dL_dz1 * last_x[j];
            b1[i] -= lr * dL_dz1;
        }
        return err * err * 0.5;
    }
} // namespace ddz
