/*
 * TestTransforms.cpp
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#include <gtest/gtest.h>
#include <stxxl/sorter>
#include "../cpp/defs.hpp"
#include "../cpp/streaming/hungdefs.hpp"
#include "../cpp/streaming/transforms/make_consecutively_filtered_stream.h"

class TestTransforms : public ::testing::Test { };

struct EqualFunctor {
    bool operator () (const edge_t& a, const edge_t& b) const {
        return a == b;
    }
};

struct EqualOrderedFunctor {
    bool operator () (const edge_t& a, const edge_t& b) const {
        const auto a1 = std::min(a.u, a.v);
        const auto a2 = std::max(a.u, a.v);
        const auto b1 = std::min(b.u, b.v);
        const auto b2 = std::max(b.u, b.v);
        return a1 == b1 && a2 == b2;
    }
};

TEST_F(TestTransforms, test_make_consecutively_filtered_stream_1) {
    stxxl::sorter<edge_t, edge_less_cmp> edges(edge_less_cmp(), SORTER_MEM);
    edges.push(edge_t{1, 2});
    edges.push(edge_t{1, 2});
    edges.push(edge_t{1, 2});
    edges.push(edge_t{2, 3});
    edges.push(edge_t{2, 3});
    edges.push(edge_t{3, 4});
    edges.sort_reuse();

    using test_type = make_consecutively_filtered_stream<decltype(edges), EqualFunctor>;
    test_type edgess_uqe(edges, EqualFunctor());
    ASSERT_EQ((*edgess_uqe).u, 1);
    ASSERT_EQ((*edgess_uqe).v, 2);
    ++edgess_uqe;
    ASSERT_EQ((*edgess_uqe).u, 2);
    ASSERT_EQ((*edgess_uqe).v, 3);
    ++edgess_uqe;
    ASSERT_EQ((*edgess_uqe).u, 3);
    ASSERT_EQ((*edgess_uqe).v, 4);
    ++edgess_uqe;
    ASSERT_TRUE(edgess_uqe.empty());
    edgess_uqe.rewind();
    ASSERT_EQ((*edgess_uqe).u, 1);
    ASSERT_EQ((*edgess_uqe).v, 2);
    ++edgess_uqe;
    ASSERT_EQ((*edgess_uqe).u, 2);
    ASSERT_EQ((*edgess_uqe).v, 3);
    ++edgess_uqe;
    ASSERT_EQ((*edgess_uqe).u, 3);
    ASSERT_EQ((*edgess_uqe).v, 4);
    ++edgess_uqe;
    ASSERT_TRUE(edgess_uqe.empty());
}

TEST_F(TestTransforms, test_make_consecutively_filtered_stream_2) {
    stxxl::sorter<edge_t, edge_less_cmp> edges(edge_less_cmp(), SORTER_MEM);
    edges.push(edge_t{1, 2});
    edges.push(edge_t{2, 1});
    edges.push(edge_t{3, 4});
    edges.push(edge_t{4, 3});
    edges.push(edge_t{5, 6});
    edges.push(edge_t{6, 5});
    edges.sort_reuse();

    using test_type = make_consecutively_filtered_stream<decltype(edges), EqualOrderedFunctor>;
    test_type edgess_uqe(edges, EqualOrderedFunctor());
    ASSERT_EQ((*edgess_uqe).u, 1);
    ASSERT_EQ((*edgess_uqe).v, 2);
    ++edgess_uqe;
    ASSERT_EQ((*edgess_uqe).u, 3);
    ASSERT_EQ((*edgess_uqe).v, 4);;
    ++edgess_uqe;
    ASSERT_EQ((*edgess_uqe).u, 5);
    ASSERT_EQ((*edgess_uqe).v, 6);
    ++edgess_uqe;
    ASSERT_TRUE(edgess_uqe.empty());
    edgess_uqe.rewind();
    ASSERT_EQ((*edgess_uqe).u, 1);
    ASSERT_EQ((*edgess_uqe).v, 2);
    ++edgess_uqe;
    ASSERT_EQ((*edgess_uqe).u, 3);
    ASSERT_EQ((*edgess_uqe).v, 4);
    ++edgess_uqe;
    ASSERT_EQ((*edgess_uqe).u, 5);
    ASSERT_EQ((*edgess_uqe).v, 6);
    ++edgess_uqe;
    ASSERT_TRUE(edgess_uqe.empty());
}