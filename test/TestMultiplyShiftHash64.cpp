/*
 * TestMultiplyShiftHash.cpp
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#include <gtest/gtest.h>
#include "../cpp/streaming/distinct_elements/multiply_shift_hash.h"

class TestMultiplyShiftHash32 : public ::testing::Test { };

class TestMultiplyShiftHash64 : public ::testing::Test { };

TEST_F(TestMultiplyShiftHash64, test1) {
    std::random_device rd;
    std::mt19937_64 gen(rd());

    MultiplyShiftHash64Bit mshash(gen);
    std::cout << mshash(0) << std::endl;
    std::cout << mshash(1) << std::endl;
    std::cout << mshash(2) << std::endl;
    std::cout << mshash(3) << std::endl;
}

TEST_F(TestMultiplyShiftHash64, test2) {
    std::random_device rd;
    std::mt19937_64 gen(rd());

    MultiplyShiftHash64Bit mshash(gen, 1ull << 40u);
    std::cout << mshash(0) << std::endl;
    std::cout << mshash(1) << std::endl;
    std::cout << mshash(2) << std::endl;
    std::cout << mshash(3) << std::endl;
}

TEST_F(TestMultiplyShiftHash32, test1) {
    std::random_device rd;
    std::mt19937_64 gen(rd());

    MultiplyShiftHash32Bit mshash(gen);
    std::cout << mshash(0) << std::endl;
    std::cout << mshash(1) << std::endl;
    std::cout << mshash(2) << std::endl;
    std::cout << mshash(3) << std::endl;
}

TEST_F(TestMultiplyShiftHash32, test2) {
    std::random_device rd;
    std::mt19937_64 gen(rd());

    MultiplyShiftHash32Bit mshash(gen, 1ull << 20u);
    std::cout << mshash(0) << std::endl;
    std::cout << mshash(1) << std::endl;
    std::cout << mshash(2) << std::endl;
    std::cout << mshash(3) << std::endl;
}