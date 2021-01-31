/*
 * TestEdgeSequence.cpp
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#include <gtest/gtest.h>
#include "../cpp/defs.hpp"
#include "../cpp/streaming/containers/EdgeSequence.h"

class TestEdgeSequence : public ::testing::Test { };

TEST_F(TestEdgeSequence, test_1) {
    EdgeSequence es;
    es.push(edge_t{1, 2});
    es.push(edge_t{2, 4});
    es.push(edge_t{3, 6});
    es.rewind();
    auto es1 = *es;
    ASSERT_EQ(1, es1.u);
    ASSERT_EQ(2, es1.v);
    ++es;

    ASSERT_FALSE(es.empty());
    auto es2 = *es;
    ASSERT_EQ(2, es2.u);
    ASSERT_EQ(4, es2.v);
    ++es;

    ASSERT_FALSE(es.empty());
    auto es3 = *es;
    ASSERT_EQ(3, es3.u);
    ASSERT_EQ(6, es3.v);
    ++es;

    ASSERT_TRUE(es.empty());
}