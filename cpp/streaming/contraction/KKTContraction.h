/*
 * KKTContraction.h
 *
 * Used in an implementation according to paradigm first used by Karger, Klein and Tarjan.
 * 1. three node contractions (Chin et. al, parallel version of Boruvka) n -> n/8
 * 2. sample half the edges, solve connected components for one half recursively
 * 3. filter computed connected components against other half
 * 4. recursively solve for remainder
 *
 * Implements step 1.
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#pragma once

#include <stxxl/sorter>
#include "BoruvkaContraction.h"
#include "../basecase/PipelinedKruskal.h"
#include "../merging/ComponentMerger.h"
#include "../transforms/make_unique_stream.h"

class KKTContraction {
    using node_sorter_less_t            = stxxl::sorter<node_t, node_less_cmp>;
    using edge_sorter_less_t            = stxxl::sorter<edge_t, edge_less_cmp>;
    using node_cc_sorter_cc_node_less_t = stxxl::sorter<node_component_t, node_component_cc_node_less_cmp>;
    using node_cc_sorter_node_cc_less_t = stxxl::sorter<node_component_t, node_component_node_cc_less_cmp>;
protected:
    node_t node_upper_bound = 0;

public:
    KKTContraction() = default;

    template <typename EdgesIn, typename ComponentsOut>
    void compute_semi_external_contraction(EdgesIn& in_edges, ComponentsOut& node_mapping, PipelinedKruskal& kruskal, size_t) {
        tlx::unused(in_edges);
        tlx::unused(node_mapping);
        tlx::unused(kruskal);
        assert(false);
    }

    template <typename EdgesIn, typename EdgesOut, typename ComponentsOut>
    void compute_fully_external_contraction(EdgesIn& in_edges, EdgesOut& contracted_edges, ComponentsOut& node_mapping, size_t) {
        if (in_edges.empty())
            return;

        using sorted_edges_type = edge_sorter_less_t;
        using sorted_edges_unique_type = make_unique_stream<sorted_edges_type>;
        using sorted_comps_type = node_cc_sorter_node_cc_less_t ;

        BoruvkaContraction fst_contraction;
        sorted_edges_type fst_edges(edge_less_cmp(), SORTER_MEM);
        sorted_comps_type fst_ccs(node_component_node_cc_less_cmp(), SORTER_MEM);
        fst_contraction.compute_fully_external_contraction(in_edges, fst_edges, fst_ccs, 0);
        fst_edges.sort();
        node_upper_bound = fst_contraction.get_node_upper_bound();

        std::cout << "1. contraction " << fst_edges.size() << " " << fst_ccs.size() << std::endl;

        // single contraction leaves no remaining contracted edges, output retrieved cc mapping
        if (fst_edges.empty()) {
            std::cout << "no edges left after 1. contraction" << std::endl;

            // push output
            StreamPusher(fst_ccs, node_mapping);

            return;
        }

        sorted_edges_unique_type fst_edges_uqe(fst_edges);
        BoruvkaContraction snd_contraction;
        sorted_edges_type snd_edges(edge_less_cmp(), SORTER_MEM);
        sorted_comps_type snd_ccs(node_component_node_cc_less_cmp(), SORTER_MEM);
        snd_contraction.compute_fully_external_contraction(fst_edges_uqe, snd_edges, snd_ccs, 0);
        snd_edges.sort();
        node_upper_bound = snd_contraction.get_node_upper_bound();

        std::cout << "2. contraction " << snd_edges.size() << " " << snd_ccs.size() << std::endl;

        // 2x contractions leave no remaining contracted edges, output combined cc mapping
        if (snd_edges.empty()) {
            std::cout << "no edges left after 2. contraction" << std::endl;

            // resort first component map to be sorted by component
            node_cc_sorter_cc_node_less_t fst_ccs_srtd_cc_node_less(node_component_cc_node_less_cmp(), SORTER_MEM);
            StreamPusher(fst_ccs, fst_ccs_srtd_cc_node_less);
            fst_ccs_srtd_cc_node_less.sort_reuse();
            fst_ccs.finish_clear();

            // merge first + second
            ComponentMerger(fst_ccs_srtd_cc_node_less, snd_ccs, node_mapping);

            return;
        }

        sorted_edges_unique_type snd_edges_uqe(snd_edges);

        BoruvkaContraction trd_contraction;
        sorted_comps_type trd_ccs(node_component_node_cc_less_cmp(), SORTER_MEM);
        trd_contraction.compute_fully_external_contraction(snd_edges_uqe, contracted_edges, trd_ccs, 0);
        node_upper_bound = trd_contraction.get_node_upper_bound();

        std::cout << "3. contraction " << contracted_edges.size() << " " << trd_ccs.size() << std::endl;

        node_cc_sorter_cc_node_less_t fst_ccs_by_ccnode(node_component_cc_node_less_cmp(), SORTER_MEM);
        StreamPusher(fst_ccs, fst_ccs_by_ccnode);
        fst_ccs_by_ccnode.sort_reuse();

        node_cc_sorter_cc_node_less_t snd_ccs_by_ccnode(node_component_cc_node_less_cmp(), SORTER_MEM);
        StreamPusher(snd_ccs, snd_ccs_by_ccnode);
        snd_ccs_by_ccnode.sort_reuse();

        auto & snd_and_trd_ccs = snd_ccs;
        snd_and_trd_ccs.clear();

        // merge second + third
        ComponentMerger(snd_ccs_by_ccnode, trd_ccs, snd_and_trd_ccs);
        snd_and_trd_ccs.sort_reuse();

        // merge first + (second + third)
        ComponentMerger(fst_ccs_by_ccnode, snd_and_trd_ccs, node_mapping);
    }

    [[nodiscard]] node_t get_node_upper_bound() const {
        return node_upper_bound;
    }

    static bool supports_only_map_return() {
        return false;
    }

    static double get_expected_contraction_ratio_upper_bound() {
        return 0.125;
    }
};