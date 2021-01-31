/*
 * TestStreamFilter.cpp
 *
 * Created on: Apr 18, 2019
 *     Author: Hung Tran (hung@ae.cs.uni-frankfurt.de)
 */

#include <gtest/gtest.h>
#include <stxxl/sequence>
#include "../cpp/defs.hpp"
#include "../cpp/streaming/hungdefs.hpp"
#include "../cpp/streaming/utils/StreamFilter.h"

class TestStreamFilter : public ::testing::Test { };

TEST_F(TestStreamFilter, test_1_hitfilter) {
    stxxl::sequence<edge_t> edges;
    std::vector<edge_t> input_edges = {{0, 2}, {1, 4}, {2, 5}, {7, 8}};
    for (const auto & e : input_edges) {
        edges.push_back(e);
    }
    stxxl::sequence<node_t> hits;
    std::vector<node_t> input_hits = {1, 2, 3, 4, 6};
    for (const auto & h : input_hits) {
        hits.push_back(h);
    }

    struct SourceLessEqual {
        bool operator ()(const edge_t & a, const node_t & b) const {
            return a.u <= b;
        }
    };

    struct SourceEqual {
        bool operator ()(const edge_t & a, const node_t & b) const {
            return a.u == b;
        }
    };

    auto edges_stream = edges.get_stream();
    auto hits_stream = hits.get_stream();
    const SourceLessEqual less;
    const SourceEqual equal;
    using hitless_stream_type = StreamHitFilter<decltype(edges_stream), decltype(hits_stream), SourceLessEqual, SourceEqual>;
    hitless_stream_type output(edges_stream, hits_stream, less, equal);

    ASSERT_FALSE(output.empty());
    ASSERT_EQ((*output), edge_t(0, 2));
    ++output;
    ASSERT_EQ((*output), edge_t(7, 8));
    ++output;
    ASSERT_TRUE(output.empty());
}