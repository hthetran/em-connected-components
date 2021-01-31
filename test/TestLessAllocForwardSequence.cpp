/*
 * TestLessAllocForwardSequence.cpp
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#include <gtest/gtest.h>
#include "../cpp/defs.hpp"
#include "../cpp/streaming/containers/less_alloc_forward_sequence.h"

class TestLessAllocForwardSequence : public ::testing::Test { };

TEST_F(TestLessAllocForwardSequence, test_1) {
    stxxl::less_alloc_forward_sequence<node_t> seq;
    ASSERT_EQ(seq.size(), 0ul);
    for (node_t i = 0; i < 100; ++i) seq.push_back(i);
    ASSERT_EQ(seq.size(), 100ul);
    {
        auto seqstream = seq.get_stream();
        for (node_t i = 0; i < 100; ++i, ++seqstream) ASSERT_EQ(*seqstream, i);
        ASSERT_TRUE(seqstream.empty());
        std::cout << "passed seqstream" << std::endl;
    }
    seq.reset();
    ASSERT_TRUE(seq.empty());
    ASSERT_EQ(seq.size(), 0ul);
    {
        auto seqstream2 = seq.get_stream();
        ASSERT_TRUE(seqstream2.empty());
        std::cout << "passed seqstream2" << std::endl;
    }
    const node_t fill_three_blocks_size = 3 * STXXL_DEFAULT_BLOCK_SIZE(node_t) / sizeof(node_t) + 1;
    for (node_t i = 0; i < fill_three_blocks_size; ++i) seq.push_back(i);
    {
        auto seqstream3 = seq.get_stream();
        for (node_t i = 0; i < fill_three_blocks_size; ++i, ++seqstream3) {
            ASSERT_EQ(*seqstream3, i);
        }
        ASSERT_TRUE(seqstream3.empty());
        std::cout << "passed seqstream3" << std::endl;
    }
    seq.reset();
    ASSERT_TRUE(seq.empty());
    ASSERT_EQ(seq.size(), 0ul);
    const node_t fill_five_blocks_size = 5 * STXXL_DEFAULT_BLOCK_SIZE(node_t) / sizeof(node_t) + 1;
    for (node_t i = 0; i < fill_five_blocks_size; ++i) seq.push_back(i);
    {
        auto seqstream4 = seq.get_stream();
        for (node_t i = 0; i < fill_five_blocks_size; ++i, ++seqstream4) {
            ASSERT_EQ(*seqstream4, i);
        }
        ASSERT_TRUE(seqstream4.empty());
        std::cout << "passed seqstream4" << std::endl;
    }
    seq.reset();
    ASSERT_TRUE(seq.empty());
    ASSERT_EQ(seq.size(), 0ul);
    const node_t fill_half_block_size = 0.5 * STXXL_DEFAULT_BLOCK_SIZE(node_t) / sizeof(node_t);
    for (node_t i = 0; i < fill_half_block_size; ++i) seq.push_back(i);
    {
        auto seqstream5 = seq.get_stream();
        for (node_t i = 0; i < fill_half_block_size; ++i, ++seqstream5) ASSERT_EQ(*seqstream5, i);
        ASSERT_TRUE(seqstream5.empty());
        std::cout << "passed seqstream5" << std::endl;
    }
    seq.reset();
    ASSERT_TRUE(seq.empty());
    ASSERT_EQ(seq.size(), 0ul);

    for (size_t j = 0; j < 10; ++j) {
        std::cout << j << std::endl;
        const node_t fill_ten_blocks_size = 10 * STXXL_DEFAULT_BLOCK_SIZE(node_t) / sizeof(node_t) + 1;
        for (node_t i = 0; i < fill_ten_blocks_size; ++i) seq.push_back(i);
        {
            auto loopstream = seq.get_stream();
            for (node_t i = 0; i < fill_ten_blocks_size; ++i, ++loopstream) {
                ASSERT_EQ(*loopstream, i);
            }
            ASSERT_TRUE(loopstream.empty());
        }
        seq.reset();
        ASSERT_TRUE(seq.empty());
        ASSERT_EQ(seq.size(), 0ul);
    }
}