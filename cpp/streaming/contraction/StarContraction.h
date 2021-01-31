/*
 * StarContraction.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#ifndef EM_CC_STARCONTRACTION_H
#define EM_CC_STARCONTRACTION_H

#include <stxxl/sorter>
#include "../../defs.hpp"
#include "../hungdefs.hpp"
#include "../transforms/make_unique_stream.h"
#include "../utils/StreamFilter.h"
#include "../utils/StreamRandomNeighbour.h"
#include "../utils/StreamSplit.h"
#include "../basecase/PipelinedKruskal.h"

namespace StarContraction_details {
    struct Project2ndEntry {
    public:
        using value_type_in = edge_t;
        using value_type = node_t;

        value_type operator() (const value_type_in & o) const {
            return o.v;
        }
    };
}

class StarContraction {
    using node_sorter_less_t         = stxxl::sorter<node_t, node_less_cmp>;
    using edge_sorter_reverse_less_t = stxxl::sorter<edge_t, edge_reverse_less_cmp>;

public:
    StarContraction() = default;

    template <typename EdgesIn, typename ComponentsOut>
    void compute_semi_external_contraction(EdgesIn& in_edges, ComponentsOut& star_mapping, PipelinedKruskal& kruskal, size_t) {
        // TODO encapsulate
        //!!  get out-going edges
        // retrieve random out-edge for each source
        using rand_incident_edge_stream_type = StreamRandomNeighbour<EdgesIn>;
        auto & to_contract_edges = in_edges;
        rand_incident_edge_stream_type rand_incident_edges(to_contract_edges, 0.5);

        //!! break paths
        // retrieve target nodes and split them off to sorter
        using rand_incident_edge_stream_type2 = StreamSplit<rand_incident_edge_stream_type, node_sorter_less_t, StarContraction_details::Project2ndEntry>;
        using target_stream_type = node_sorter_less_t;
        target_stream_type targets(node_less_cmp(), SORTER_MEM);
        rand_incident_edge_stream_type2 rand_incident_edges2(rand_incident_edges, targets, StarContraction_details::Project2ndEntry());

        // flush out target entries and sort
        for (; !rand_incident_edges2.empty(); ++rand_incident_edges2);
        targets.sort();
        make_unique_stream<decltype(targets)> targets_uqe(targets, MAX_NODE);
        rand_incident_edges2.rewind();
        assert(rand_incident_edges2.size() == targets.size());

        // filter out-edges that are targeted
        using star_edges_stream_type = StreamHitFilter<rand_incident_edge_stream_type2, decltype(targets_uqe), edge_source_lessequal, edge_source_equal>;
        const edge_source_lessequal edge_source_node_le;
        const edge_source_equal edge_source_node_e;
        star_edges_stream_type star_edges(rand_incident_edges2, targets_uqe, edge_source_node_le, edge_source_node_e);

        //!! contract stars
        node_upper_bound = rand_incident_edges.get_num_sources();
        edge_t prev_edge(INVALID_NODE, INVALID_NODE);

        // update source nodes
        using source_updated_edges_stream_type = edge_sorter_reverse_less_t;
        to_contract_edges.rewind();
        source_updated_edges_stream_type source_updated_edges(edge_reverse_less_cmp(), SORTER_MEM);
        for (; !to_contract_edges.empty(); ++to_contract_edges) {
            const edge_t edge = *to_contract_edges;
            while (!star_edges.empty() && (*star_edges).u < edge.u) {
                const auto star_edge = *star_edges;
                star_mapping.push(node_component_t{star_edge.u, star_edge.v});
                star_mapping.push(node_component_t{star_edge.v, star_edge.v});
                ++star_edges;
            }
            if (!star_edges.empty() && (*star_edges).u == edge.u) {
                const node_t new_source = (*star_edges).v;
                if (new_source == edge.v) continue; // self loop
                source_updated_edges.push(edge_t{new_source, edge.v});
            } else {
                source_updated_edges.push(edge);
            }
        }

        // flush stream
        for (; !star_edges.empty(); ++star_edges) {
            const auto star_edge = *star_edges;
            star_mapping.push(node_component_t{star_edge.u, star_edge.v});
            star_mapping.push(node_component_t{star_edge.v, star_edge.v});
        }

        // asserts
        assert(to_contract_edges.empty());
        assert(star_edges.empty());

        // sort source updated edges
        source_updated_edges.sort();

        // update target nodes, push to subproblem edges immediately
        star_edges.rewind();
        for (; !source_updated_edges.empty(); ++source_updated_edges) {
            const edge_t edge = *source_updated_edges;
            node_upper_bound += (prev_edge.v != edge.v);

            while (!star_edges.empty() && (*star_edges).u < edge.v) ++star_edges;
            if (!star_edges.empty() && (*star_edges).u == edge.v) {
                const node_t new_target = (*star_edges).v;
                if (new_target == edge.u) continue; // self loop
                kruskal.push(edge_t{std::min(edge.u, new_target), std::max(edge.u, new_target)});
            } else {
                kruskal.push(edge_t{std::min(edge.u, edge.v), std::max(edge.u, edge.v)});
            }

            prev_edge = edge;
        }

        // asserts
        assert(source_updated_edges.empty());
    }

    template <typename EdgesIn, typename EdgesOut, typename ComponentsOut>
    void compute_fully_external_contraction(EdgesIn& in_edges, EdgesOut& contracted_edges, ComponentsOut& star_mapping, size_t) {
        //!!  get out-going edges
        // retrieve random out-edge for each source
        using rand_incident_edge_stream_type = StreamRandomNeighbour<EdgesIn>;
        auto & to_contract_edges = in_edges;
        rand_incident_edge_stream_type rand_incident_edges(to_contract_edges, 0.5);

        //!! break paths
        // retrieve target nodes and split them off to sorter
        using rand_incident_edge_stream_type2 = StreamSplit<rand_incident_edge_stream_type, node_sorter_less_t, StarContraction_details::Project2ndEntry>;
        using target_stream_type = node_sorter_less_t;
        target_stream_type targets(node_less_cmp(), SORTER_MEM);
        rand_incident_edge_stream_type2 rand_incident_edges2(rand_incident_edges, targets, StarContraction_details::Project2ndEntry());

        // flush out target entries and sort
        for (; !rand_incident_edges2.empty(); ++rand_incident_edges2);
        targets.sort();
        make_unique_stream<decltype(targets)> targets_uqe(targets, MAX_NODE);
        rand_incident_edges2.rewind();
        assert(rand_incident_edges2.size() == targets.size());

        // filter out-edges that are targeted
        using star_edges_stream_type = StreamHitFilter<rand_incident_edge_stream_type2, decltype(targets_uqe), edge_source_lessequal, edge_source_equal>;
        const edge_source_lessequal edge_source_node_le;
        const edge_source_equal edge_source_node_e;
        star_edges_stream_type star_edges(rand_incident_edges2, targets_uqe, edge_source_node_le, edge_source_node_e);

        //!! contract stars
        node_upper_bound = rand_incident_edges.get_num_sources();
        edge_t prev_edge(INVALID_NODE, INVALID_NODE);

        // update source nodes
        edge_sorter_reverse_less_t source_updated_edges(edge_reverse_less_cmp(), SORTER_MEM);
        to_contract_edges.rewind();
        for (; !to_contract_edges.empty(); ++to_contract_edges) {
            const edge_t edge = *to_contract_edges;
            while (!star_edges.empty() && (*star_edges).u < edge.u) {
                const auto star_edge = *star_edges;
                star_mapping.push(node_component_t{star_edge.u, star_edge.v});
                star_mapping.push(node_component_t{star_edge.v, star_edge.v});
                ++star_edges;
            }
            if (!star_edges.empty() && (*star_edges).u == edge.u) {
                const node_t new_source = (*star_edges).v;
                if (new_source == edge.v) continue; // self loop
                source_updated_edges.push(edge_t{new_source, edge.v});
            } else {
                source_updated_edges.push(edge);
            }
        }

        // asserts
        assert(to_contract_edges.empty());

        // flush star edges
        for (; !star_edges.empty(); ++star_edges) {
            const auto star_edge = *star_edges;
            star_mapping.push(node_component_t{star_edge.u, star_edge.v});
            star_mapping.push(node_component_t{star_edge.v, star_edge.v});
        }

        // sort source updated edges
        source_updated_edges.sort();

        // update target nodes, push to subproblem edges immediately
        star_edges.rewind();
        for (; !source_updated_edges.empty(); ++source_updated_edges) {
            const edge_t edge = *source_updated_edges;
            node_upper_bound += (prev_edge.v != edge.v);

            while (!star_edges.empty() && (*star_edges).u < edge.v) {
                ++star_edges;
            }
            if (!star_edges.empty() && (*star_edges).u == edge.v) {
                const node_t new_target = (*star_edges).v;
                if (new_target == edge.u) continue; // self loop
                contracted_edges.push(edge_t{edge.u, new_target}.normalized());
            } else {
                contracted_edges.push(edge.normalized());
            }

            prev_edge = edge;
        }

        // asserts
        assert(source_updated_edges.empty());
    }

    [[nodiscard]] node_t get_node_upper_bound() const {
        return node_upper_bound;
    }

    static bool supports_only_map_return() {
        return true;
    }

    static double get_expected_contraction_ratio_upper_bound() {
        return 0.75;
    }

private:
    node_t node_upper_bound = 0;
};

#endif //EM_CC_STARCONTRACTION_H
