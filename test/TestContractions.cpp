/*
 * TestContractions.cpp
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#include <gtest/gtest.h>
#include <stxxl/sequence>
#include <stxxl/sorter>
#include <stxxl/stream>
#include "../cpp/streaming/contraction/StarContraction.h"
#include "../cpp/streaming/containers/EdgeSequence.h"
#include "../cpp/streaming/contraction/BoruvkaContraction.h"

class TestBoruvkaContraction : public ::testing::TestWithParam<node_t> { };

TEST_P(TestBoruvkaContraction, check_matching_correctness) {
    EdgeSequence edges;
    const node_t problem_size = GetParam();
    for (node_t i = 1; i < 1 + problem_size * 2; i = i + 2) edges.push(edge_t{i, i + 1});
    edges.rewind();

    BoruvkaContraction boruvka_algo;
    stxxl::sorter<edge_t, edge_less_cmp> contracted_edges(edge_less_cmp(), SORTER_MEM);
    stxxl::sorter<node_component_t, node_component_node_cc_less_cmp> cc_map(node_component_node_cc_less_cmp(), SORTER_MEM);
    boruvka_algo.compute_fully_external_contraction(edges, contracted_edges, cc_map, 0);

    // test contracted edges
    contracted_edges.sort_reuse();
    ASSERT_TRUE(contracted_edges.empty());

    // test cc map
    for (node_t i = 1; i < 1 + problem_size * 2; i = i + 2) {
        const auto cc1 = *cc_map; ++cc_map;
        const auto cc2 = *cc_map; ++cc_map;
        ASSERT_EQ(cc1.node, i);
        ASSERT_EQ(cc2.node, i+1);
        ASSERT_EQ(cc1.load, cc2.load);
        ASSERT_TRUE(cc1.load == i || cc1.load == i+1);
    }
    assert(cc_map.empty());
}

TEST_P(TestBoruvkaContraction, check_path_correctness) {
    EdgeSequence edges;
    const node_t problem_size = GetParam();
    for (node_t i = 1; i <= problem_size; ++i) edges.push(edge_t{i, i + 1});
    edges.rewind();

    BoruvkaContraction boruvka_algo;
    stxxl::sorter<edge_t, edge_less_cmp> contracted_edges(edge_less_cmp(), SORTER_MEM);
    stxxl::sorter<node_component_t, node_component_node_cc_less_cmp> cc_map(node_component_node_cc_less_cmp(), SORTER_MEM);
    boruvka_algo.compute_fully_external_contraction(edges, contracted_edges, cc_map, 0);

    // test contracted edges
    contracted_edges.sort_reuse();
    ASSERT_TRUE(contracted_edges.empty());

    // test cc map
    const auto fstload = (*cc_map).load;
    ASSERT_TRUE(fstload == 1 || fstload == 2);
    for (node_t i = 1; i <= problem_size + 1; ++i) {
        const auto cc1 = *cc_map; ++cc_map;
        ASSERT_EQ(cc1.node, i);
        ASSERT_EQ(cc1.load, fstload);
    }
    assert(cc_map.empty());
}

INSTANTIATE_TEST_SUITE_P(
Contractions,
TestBoruvkaContraction,
::testing::Values(1u<<3, 1u<<10, 1u<<14, 1u<<18)
);

class TestStreamSplit : public ::testing::Test { };

TEST_F(TestStreamSplit, test_1_split_off_random_edge_with_targets) {
    using node_sorter_less_t = stxxl::sorter<node_t, node_less_cmp>;

    stxxl::sequence<edge_t> edges;
    const node_t degree = 3;
    const node_t num_nodes = 1000;
    for (node_t i = 1; i < num_nodes; i++) {
        for (node_t j = i + 1; j < i + 1 + degree; j++) {
            edges.push_back(edge_t{i, j});
        }
    }
    edges.push_back(edge_t{0, num_nodes + 1});

    auto edge_stream = edges.get_stream();

    // retrieve random incident node
    using rand_incident_edge_stream_type = StreamRandomNeighbour<decltype(edge_stream)>;
    rand_incident_edge_stream_type rand_incident_edge_stream(edge_stream, 1.    );

    // split off random incident nodes to sort separately
    using rand_incident_edge_stream_type2 = StreamSplit<rand_incident_edge_stream_type, node_sorter_less_t, StarContraction_details::Project2ndEntry>;
    node_sorter_less_t targets(node_less_cmp(), SORTER_MEM);
    const StarContraction_details::Project2ndEntry callback;
    rand_incident_edge_stream_type2 rand_incident_edge_stream2(rand_incident_edge_stream, targets, callback);

    // sort original stream and split off one
    using rand_incident_edge_sort_stream_type = stxxl::stream::sort<rand_incident_edge_stream_type2, edge_reverse_less_cmp>;
    rand_incident_edge_sort_stream_type rand_incident_edge_sort_stream(rand_incident_edge_stream2, edge_reverse_less_cmp(), SORTER_MEM);

    // the elements must be the same
    targets.sort();
    auto zip_stream = stxxl::stream::make_tuplestream<rand_incident_edge_sort_stream_type, node_sorter_less_t>(rand_incident_edge_sort_stream, targets);
    for (; !zip_stream.empty(); ++zip_stream) {
        const auto zip_entry = *zip_stream;
        ASSERT_EQ(std::get<0>(zip_entry).v, std::get<1>(zip_entry));
    }
}
