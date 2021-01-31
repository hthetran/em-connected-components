/*
 * TestStreamRandomNeighbour.cpp
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#include <gtest/gtest.h>
#include <stxxl/sequence>
#include "../cpp/streaming/utils/StreamRandomNeighbour.h"

static std::pair<double, double> binomial_mean_stddev(size_t n, double p) {
    return std::make_pair(n * p, std::sqrt(static_cast<double>(n) * p * (1 - p)));
}

class TestStreamRandomNeighbour : public ::testing::Test { };

TEST_F(TestStreamRandomNeighbour, test_1_single_entry) {
    stxxl::sequence<edge_t> edges;
    edges.push_back(edge_t{0, 1});

    auto edges_stream = edges.get_stream();
    StreamRandomNeighbour<decltype(edges_stream)> random_neighbour_stream(edges_stream, 1.);
    ASSERT_FALSE(random_neighbour_stream.empty());
    ASSERT_EQ((*random_neighbour_stream), edge_t(0, 1));
    ++random_neighbour_stream;
    ASSERT_TRUE(random_neighbour_stream.empty());
    ++random_neighbour_stream;
    ASSERT_TRUE(random_neighbour_stream.empty());
}

TEST_F(TestStreamRandomNeighbour, test_2_single_source) {
    stxxl::sequence<edge_t> edges;
    edges.push_back(edge_t{0, 1});
    edges.push_back(edge_t{0, 3});
    edges.push_back(edge_t{0, 5});
    edges.push_back(edge_t{0, 6});

    const size_t testing_size = 1000;
    size_t count_1 = 0;
    for (size_t i = 0; i < testing_size; i++) {
        auto edges_stream = edges.get_stream();
        StreamRandomNeighbour<decltype(edges_stream)> random_neighbour_stream(edges_stream, 1.);

        for (; !random_neighbour_stream.empty(); ++random_neighbour_stream) {
            const auto edge = *random_neighbour_stream;

            if (edge.u == 0 && edge.v == 1) {
                count_1++;
            }
        }
    }

    auto values_1 = binomial_mean_stddev(testing_size, 0.25);
    EXPECT_LE(count_1, values_1.first + 3 * values_1.second);
    EXPECT_GE(count_1, values_1.first - 3 * values_1.second);
}

TEST_F(TestStreamRandomNeighbour, test_3_multiple_sources_correctness) {
    stxxl::sequence<edge_t> edges;
    edges.push_back(edge_t{0, 1});
    edges.push_back(edge_t{0, 3});
    edges.push_back(edge_t{0, 5});
    edges.push_back(edge_t{0, 6});
    edges.push_back(edge_t{1, 2});
    edges.push_back(edge_t{1, 7});
    edges.push_back(edge_t{2, 8});

    auto edges_stream = edges.get_stream();
    StreamRandomNeighbour<decltype(edges_stream)> random_neighbour_stream(edges_stream, 0.5);

    // flush elements
    std::vector<edge_t> output_edges;
    for (; !random_neighbour_stream.empty(); ++random_neighbour_stream) {
        output_edges.push_back(*random_neighbour_stream);
    }

    random_neighbour_stream.rewind();
    for (size_t i = 0; i < output_edges.size(); i++) {
        ASSERT_EQ(*random_neighbour_stream, output_edges[i]);
        ++random_neighbour_stream;
    }
    ASSERT_TRUE(random_neighbour_stream.empty());
}

TEST_F(TestStreamRandomNeighbour, test_4_multiple_sources_probability) {
    stxxl::sequence<edge_t> edges;
    edges.push_back(edge_t{0, 1});
    edges.push_back(edge_t{0, 3});
    edges.push_back(edge_t{0, 5});
    edges.push_back(edge_t{0, 6});
    edges.push_back(edge_t{1, 2});
    edges.push_back(edge_t{1, 7});
    edges.push_back(edge_t{2, 8});

    const size_t testing_size = 1000;
    size_t count_1 = 0;
    size_t count_2 = 0;
    size_t count_3 = 0;
    for (size_t i = 0; i < testing_size; i++) {
        auto edges_stream = edges.get_stream();
        StreamRandomNeighbour<decltype(edges_stream)> random_neighbour_stream(edges_stream, 1.);

        for (; !random_neighbour_stream.empty(); ++random_neighbour_stream) {
            const auto edge = *random_neighbour_stream;

            if (edge.u == 0 && edge.v == 1) {
                count_1++;
            }
            if (edge.u == 1 && edge.v == 2) {
                count_2++;
            }
            if (edge.u == 2) {
                count_3++;
            }
        }
    }

    auto values_1 = binomial_mean_stddev(testing_size, 0.25 );
    auto values_2 = binomial_mean_stddev(testing_size, 0.5);
    EXPECT_LE(count_1, values_1.first + 3 * values_1.second);
    EXPECT_GE(count_1, values_1.first - 3 * values_1.second);
    EXPECT_LE(count_2, values_2.first + 3 * values_2.second);
    EXPECT_GE(count_2, values_2.first - 3 * values_2.second);
    ASSERT_EQ(count_3, testing_size);
}

TEST_F(TestStreamRandomNeighbour, test_5_multiple_sources_edge_selection) {
    stxxl::sequence<edge_t> edges;
    edges.push_back(edge_t{0, 1});
    edges.push_back(edge_t{0, 3});
    edges.push_back(edge_t{0, 5});
    edges.push_back(edge_t{0, 6});

    const size_t testing_size = 1000;
    size_t count_empty = 0;
    for (size_t i = 0; i < testing_size; i++) {
        auto edges_stream = edges.get_stream();
        StreamRandomNeighbour<decltype(edges_stream)> random_neighbour_stream(edges_stream, 0.4);
        count_empty += !random_neighbour_stream.empty();
    }

    auto values_1 = binomial_mean_stddev(testing_size, 0.4);
    EXPECT_LE(count_empty, values_1.first + 3 * values_1.second);
    EXPECT_GE(count_empty, values_1.first - 3 * values_1.second);
}

TEST_F(TestStreamRandomNeighbour, test_6_path) {
    stxxl::sequence<edge_t> edges;
    const size_t path_size = 256;
    for (node_t i = 0; i < path_size; i++) {
        edges.push_back(edge_t{i, i+1});
    }

    const size_t testing_size = 100;
    size_t count_sampled_random_neighbours = 0;
    for (size_t i = 0; i < testing_size; i++) {
        auto edges_stream = edges.get_stream();
        StreamRandomNeighbour<decltype(edges_stream)> random_neighbour_stream(edges_stream, 0.5);
        for (; !random_neighbour_stream.empty(); ++random_neighbour_stream);
        count_sampled_random_neighbours += random_neighbour_stream.size();
    }

    auto values_1 = binomial_mean_stddev(path_size, 0.5);
    EXPECT_LE(count_sampled_random_neighbours / testing_size, values_1.first + 3 * values_1.second);
    EXPECT_GE(count_sampled_random_neighbours / testing_size, values_1.first - 3 * values_1.second);
}