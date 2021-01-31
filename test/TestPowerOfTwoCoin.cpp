/*
 * TestPowerOfTwoCoin.cpp
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#include <gtest/gtest.h>
#include <random>
#include "../cpp/streaming/utils/PowerOfTwoCoin.h"

class TestPowerOfTwoCoin : public ::testing::Test { };

TEST_F(TestPowerOfTwoCoin, test1) {
    std::random_device rd;
    std::mt19937_64 gen(rd());

    const size_t n = 1u << 21u;
    for (size_t i = 1; i < 7; ++i) {
        PowerOfTwoCoin p2(i);

        size_t t = 0;
        for (size_t j = 0; j < n; ++j) {
            t += p2(gen);
        }

        const double p = p2.get_probability();
        const size_t stddev = std::sqrt(n * p * (1 - p));
        EXPECT_LE(t, n * p + 3 * stddev);
        EXPECT_GE(t, n * p - 3 * stddev);
    }
}