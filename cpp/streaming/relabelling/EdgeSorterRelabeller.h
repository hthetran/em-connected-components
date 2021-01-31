/*
 * EdgeSorterRelabeller.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#pragma once

#include <type_traits>
#include "../transforms/make_unique_stream.h"

class EdgeSorterSourceRelabeller {
public:
    template <
    typename NodeMapSorter,
    typename InEdges,
    typename OutEdges
    >
    EdgeSorterSourceRelabeller(NodeMapSorter& map, InEdges& edges, OutEdges& updated_edges, bool skip_self_loops = true) {
        static_assert(std::is_same<typename NodeMapSorter::value_type, node_component_t>::value,
                      "Sorter requires value_type that contains (node, load).");
        static_assert(std::is_same<typename NodeMapSorter::cmp_type, node_component_node_cc_less_cmp>::value,
                      "Sorter requires cmp_type that sorts by (node).");
        static_assert(std::is_same<typename InEdges::value_type, edge_t>::value,
                      "Edges require value_type (node, node).");
        static_assert(std::is_same<typename OutEdges::value_type, edge_t>::value,
                      "Edges require value_type (node, node).");
        static_assert(std::is_same<typename OutEdges::cmp_type, edge_reverse_less_cmp>::value,
                      "Updated edges are required to be sorted by target (node).");

        relabel(map, edges, updated_edges, skip_self_loops, [](const auto & map_entry){ tlx::unused(map_entry); return; });
    }

    template <
    typename NodeMapSorter,
    typename OutNodeMapSorter,
    typename InEdges,
    typename OutEdges
    >
    EdgeSorterSourceRelabeller(NodeMapSorter& map, OutNodeMapSorter& out_map, InEdges& edges, OutEdges& updated_edges, bool skip_self_loops = true) {
        static_assert(std::is_same<typename NodeMapSorter::value_type, node_component_t>::value,
                      "Sorter requires value_type that contains (node, load).");
        static_assert(std::is_same<typename NodeMapSorter::cmp_type, node_component_node_cc_less_cmp>::value,
                      "Sorter requires cmp_type that sorts by (node).");
        static_assert(std::is_same<typename OutNodeMapSorter::value_type, node_component_t>::value,
                      "Sorter requires value_type that contains (node, load).");
        static_assert(std::is_same<typename OutNodeMapSorter::cmp_type, node_component_cc_node_less_cmp>::value,
                      "Sorter requires cmp_type that sorts by (cc, node).");
        static_assert(std::is_same<typename InEdges::value_type, edge_t>::value,
                      "Edges require value_type (node, node).");
        static_assert(std::is_same<typename OutEdges::value_type, edge_t>::value,
                      "Edges require value_type (node, node).");
        static_assert(std::is_same<typename OutEdges::cmp_type, edge_reverse_less_cmp>::value,
                      "Updated edges are required to be sorted by target (node).");

        relabel(map, edges, updated_edges, skip_self_loops, [& out_map](const auto & map_entry) { out_map.push(map_entry); });
    }

private:
    template <
    typename NodeMapSorter,
    typename InEdges,
    typename OutEdges,
    typename Callback
    >
    void relabel(NodeMapSorter& map, InEdges& edges, OutEdges& updated_edges, bool skip_self_loops, Callback cb) {
        using map_unique_type = make_unique_stream<NodeMapSorter>;
        map_unique_type map_uqe(map);

        for (; !map_uqe.empty(); ++map_uqe) {
            const auto map_entry = *map_uqe;
            cb(map_entry);
            for (; !edges.empty(); ++edges) {
                const auto edge = *edges;
                if (edge.u < map_entry.node) {
                    updated_edges.push(edge);
                } else if (edge.u == map_entry.node) {
                    if (map_entry.load == edge.v && skip_self_loops) continue;
                    updated_edges.push(edge_t{map_entry.load, edge.v});
                } else
                    break;
            }
        }

        // flush out edges
        for (; !edges.empty(); ++edges) {
            const auto edge = *edges;
            updated_edges.push(edge);
        };

        // asserts
        assert(edges.empty());
    }
};

class EdgeSorterTargetRelabeller {
public:
    template <
    typename NodeMapSorter,
    typename InEdges,
    typename OutStream
    >
    EdgeSorterTargetRelabeller(NodeMapSorter& map, InEdges& edges, OutStream& updated_edges, bool skip_self_loops = true) {
        static_assert(std::is_same<typename NodeMapSorter::value_type, node_component_t>::value,
                      "Sorter requires value_type that contains (node, load).");
        static_assert(std::is_same<typename NodeMapSorter::cmp_type, node_component_node_cc_less_cmp>::value,
                      "Sorter requires cmp_type that sorts by (node).");
        static_assert(std::is_same<typename InEdges::value_type, edge_t>::value,
                      "Edges require value_type (node, node).");

        using map_unique_type = make_unique_stream<NodeMapSorter>;
        map_unique_type map_uqe(map);

        for (; !map_uqe.empty(); ++map_uqe) {
            const auto map_entry = *map_uqe;
            for (; !edges.empty(); ++edges) {
                const auto edge = *edges;
                die_unless_valid_edge(edge);

                if (edge.v < map_entry.node) {
                    die_unless_valid_edge(edge);
                    updated_edges.push(edge.normalized());
                } else if (edge.v == map_entry.node) {
                    if (map_entry.load == edge.u && skip_self_loops) continue;
                    const auto new_edge = edge_t{edge.u, map_entry.load}.normalized();
                    die_unless_valid_edge(new_edge);
                    updated_edges.push(new_edge);
                } else
                    break;
            }
        }

        // flush out edges
        for (; !edges.empty(); ++edges) {
            const auto edge = *edges;
            die_unless_valid_edge(edge);
            updated_edges.push(edge.normalized());
        }

        // asserts
        assert(edges.empty());
    }
};