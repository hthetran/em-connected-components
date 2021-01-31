/*
 * FunctionalSubproblemManager.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 *               2020 David Hammer
 *               2020 Manuel Penschuck
 */
#pragma once

//#define VERIFY_CC_REPR
//#define VERIFY_CC_MERGE
//#define VERIFY_CC_STARS

//#define HASH_ESTIMATE_1
//#define HASH_ESTIMATE_2
//#define EXPLICIT_COUNT

#if defined(HASH_ESTIMATE_1) && defined(HASH_ESTIMATE_2)
#error "Use at most one of the two settings for amount of hashing in estimate"
#endif

#if !defined(NDEBUG) || defined(EXPLICIT_COUNT)
#include "../robin_hood.h"
#endif

#include <cmath>
#include <stxxl/sorter>
#include "../defs.hpp"
#include "hungdefs.hpp"
#include "containers/EdgeStream.h"
#include "basecase/PipelinedKruskal.h"
#include "basecase/StreamKruskal.h"
#include "merging/ComponentMerger.h"
#include "relabelling/EdgeSorterRelabeller.h"
#include "transforms/make_unique_stream.h"
#include "utils/StreamPusher.h"
#include "utils/StreamRandomNeighbour.h"
#include "utils/StreamSplit.h"
#include "utils/PowerOfTwoCoin.h"
#include "distinct_elements/ApplyMeans.h"
#include "distinct_elements/ApplyMedians.h"
#include "distinct_elements/Tidemark.h"
#include "distinct_elements/MinSketch.h"
#include "distinct_elements/KSummary.h"
#include "../variants.hpp"

class foxxll_timer {
private:
    const std::string label;
    foxxll::stats* stats;
    const foxxll::stats_data stats_begin;
public:
    explicit foxxll_timer(std::string label_) : label(std::move(label_)), stats(foxxll::stats::get_instance()), stats_begin(*stats) { };
    ~foxxll_timer() { std::cout << label << ": " << (foxxll::stats_data(*stats) - stats_begin).get_elapsed_time() << std::endl; }
};

template <typename EdgesIn, typename Contraction>
class FunctionalSubproblemManager {
    // edge stream types
    using edge_sequence_t               = EdgeStream;
    using edge_sorter_less_t            = stxxl::sorter<edge_t, edge_less_cmp>;
    using edge_sorter_reverse_less_t    = stxxl::sorter<edge_t, edge_reverse_less_cmp>;

    // component label stream types
    using node_cc_sorter_node_cc_less_t = stxxl::sorter<node_component_t, node_component_node_cc_less_cmp>;
    using node_cc_sorter_cc_node_less_t = stxxl::sorter<node_component_t, node_component_cc_node_less_cmp>;
    using cc_node_less_unique_type      = make_unique_stream<node_cc_sorter_node_cc_less_t>;
    using unique_cc_stream_t            = make_unique_stream<stxxl::sorter<node_component_t, node_component_node_cc_less_cmp>>;

private:
    EdgesIn& edges;
    const size_t num_edges;
    const node_t num_nodes;
    const size_t main_memory_size;

    // data structures for the algorithm
    std::mt19937_64 gen;
    std::vector<std::unique_ptr<edge_sequence_t>> sub_edges_levels;
    std::vector<std::unique_ptr<node_cc_sorter_node_cc_less_t>> ccs_left;
    std::vector<std::unique_ptr<node_cc_sorter_node_cc_less_t>> ccs_right;
    std::unique_ptr<unique_cc_stream_t> output_ccs;
    size_t level = 0;
    size_t latest_max_level = 0;
    size_t used_main_memory_size = 0;
    node_component_t last_output{MAX_NODE, MAX_NODE};
    policy_t& policy;

public:
    FunctionalSubproblemManager() = delete;

    FunctionalSubproblemManager(EdgesIn& edges, size_t main_memory_size, node_t num_nodes, policy_t& policy, unsigned seed = std::random_device()())
	: edges(edges),
      num_edges(edges.size()),
      num_nodes(num_nodes),
      main_memory_size(main_memory_size),
      gen(seed),
      sub_edges_levels(),
      policy(policy)
    {
	    std::cout << "Instantiated FunctionalSubproblemManager" << std::endl;
        sub_edges_levels.emplace_back(new edge_sequence_t());
        sub_edges_levels.emplace_back(new edge_sequence_t());
        ccs_left.emplace_back(new node_cc_sorter_node_cc_less_t(node_component_node_cc_less_cmp(), SORTER_MEM));
        ccs_right.emplace_back(new node_cc_sorter_node_cc_less_t(node_component_node_cc_less_cmp(), SORTER_MEM));

        process(edges, num_nodes, level, true);
        output_ccs = std::make_unique<unique_cc_stream_t>(*ccs_left[0]);

        used_main_memory_size += 2*SORTER_MEM;
    }

    [[nodiscard]] bool empty() const {
        return output_ccs->empty();
    }

    node_component_t operator* () const {
        return output_ccs->operator*();
    }

    FunctionalSubproblemManager& operator++ () {
        output_ccs->operator++();
        for (; !output_ccs->empty(); output_ccs->operator++()) {
            const auto curr_output = output_ccs->operator*();
            if (curr_output.node == last_output.node) {
                assert(curr_output.load == last_output.load);
                last_output = curr_output;
            } else {
                last_output = curr_output;
                break;
            }
        }
        return *this;
    }

    void rewind() {
        last_output = node_component_t{MAX_NODE, MAX_NODE};
        output_ccs->rewind();
    }

    // TODO Add assertions that everything is empty before destroying
    ~FunctionalSubproblemManager() {
        for (auto & sub_edges : sub_edges_levels) sub_edges.reset(nullptr);
        for (auto & sub_ccs : ccs_left)  sub_ccs.reset(nullptr);
        for (auto & sub_ccs : ccs_right) sub_ccs.reset(nullptr);
    }

private:

    template <typename InEdges, typename OutComponentsSorter>
    std::pair<node_t, node_t> semi_external(InEdges& in_edges, OutComponentsSorter& ccs_out) {
        foxxll_timer basecase_timer("Basecase");

        using in_edges_unique_type = make_unique_stream<InEdges>;
        in_edges_unique_type in_edges_uqe(in_edges);
        StreamKruskal semiext_kruskal_algo;
        semiext_kruskal_algo.process(ccs_out, in_edges_uqe);
        ccs_out.sort_reuse();

        assert(in_edges_uqe.empty());

        return std::make_pair(semiext_kruskal_algo.get_num_nodes(), semiext_kruskal_algo.get_num_ccs());
    }

    template <typename InEdges, typename OutComponentsSorter>
    std::pair<node_t, node_t> semi_external(InEdges& in_edges_left, InEdges& in_edges_right, OutComponentsSorter& ccs_out) {
        foxxll_timer basecase_timer("Basecase");

        using in_edges_unique_type = make_unique_stream<InEdges>;
        in_edges_unique_type in_edges_left_uqe(in_edges_left);
        in_edges_unique_type in_edges_right_uqe(in_edges_right);
        StreamKruskal semiext_kruskal_algo;
        semiext_kruskal_algo.process(ccs_out, in_edges_left_uqe, in_edges_right_uqe);
        ccs_out.sort_reuse();

        assert(in_edges_left_uqe.empty());
        assert(in_edges_right_uqe.empty());

        return std::make_pair(semiext_kruskal_algo.get_num_nodes(), semiext_kruskal_algo.get_num_ccs());
    }

    node_t relabel_right_edges(size_t current_level, node_cc_sorter_cc_node_less_t& ccs_G_ip1_left_srtd_cc_node_less, edge_sorter_less_t& edges_G_ip1_right_over_left) {
        node_t node_upp_bnd_G_ip1_right_relabel = 0;

        // retrieve edges of right subcall
        auto & edges_G_ip1_right = *sub_edges_levels[current_level];

        // use components of left subcall
        auto & ccs_G_ip1_left    = *ccs_left[current_level + 1];
        make_unique_stream<node_cc_sorter_node_cc_less_t> ccs_G_ip1_left_uqe(ccs_G_ip1_left);

        // reset
        reset_edges(current_level + 1);

        //!! update right subproblem with ccs of left subproblem, save ccs of left but sorted by comp
        foxxll_timer relabelling_timer("Relabelling");
        std::cout << "Relabelling Sources (Components Left: " << ccs_G_ip1_left.size() << ")"
                  << " to (Edges: " << edges_G_ip1_right.size() << ")" << std::endl;

        // update sources first
        std::cout << "  updating sources" << std::endl;
        edge_sorter_reverse_less_t edges_G_ip1_right_upsrc(edge_reverse_less_cmp(), SORTER_MEM);
        node_t last_src_G_ip1{MAX_NODE};
        for (; !ccs_G_ip1_left_uqe.empty(); ++ccs_G_ip1_left_uqe) {
            const auto node_cc_G_i_left = *ccs_G_ip1_left_uqe;
            ccs_G_ip1_left_srtd_cc_node_less.push(node_cc_G_i_left);
            for (; !edges_G_ip1_right.empty(); ++edges_G_ip1_right) {
                const auto edge_G_ip1_right = *edges_G_ip1_right;
                node_upp_bnd_G_ip1_right_relabel += (last_src_G_ip1 != edge_G_ip1_right.u);
                last_src_G_ip1 = edge_G_ip1_right.u;
                if (edge_G_ip1_right.u < node_cc_G_i_left.node) {
                    assert(edge_G_ip1_right.u != edge_G_ip1_right.v);
                    edges_G_ip1_right_upsrc.push(edge_G_ip1_right);
                } else if (edge_G_ip1_right.u == node_cc_G_i_left.node) {
                    // skip self-loop
                    if (node_cc_G_i_left.load == edge_G_ip1_right.v) continue;
                    edges_G_ip1_right_upsrc.push(edge_t{node_cc_G_i_left.load, edge_G_ip1_right.v});
                } else
                    break;
            }
        }

        // flush edges
        std::cout << "  flushing remaining edges" << std::endl;
        for (; !edges_G_ip1_right.empty(); ++edges_G_ip1_right) {
            const auto edge_G_ip1_right = *edges_G_ip1_right;
            node_upp_bnd_G_ip1_right_relabel += (last_src_G_ip1 != edge_G_ip1_right.u);
            last_src_G_ip1 = edge_G_ip1_right.u;
            edges_G_ip1_right_upsrc.push(edge_G_ip1_right);
        }

        // no longer need non-updated edges
        assert(edges_G_ip1_right.empty());
        reset_edges(current_level);

        // sort source updated edges
        std::cout << "  sorting source updated edges" << std::endl;
        edges_G_ip1_right_upsrc.sort_reuse();

        // update targets next
        make_unique_stream<decltype(edges_G_ip1_right_upsrc)> edges_G_ip1_right_upsrc_uqe(edges_G_ip1_right_upsrc, edge_t{MAX_NODE, MAX_NODE});
        ccs_G_ip1_left_uqe.rewind();
        node_t last_target_G_ip1{MAX_NODE};
        std::cout << "Relabelling Targets (Components Left: " << ccs_G_ip1_left.size() << ")"
                  << " to (Source Updated Edges: " << edges_G_ip1_right_upsrc.size() << ")" << std::endl;
        std::cout << "  updating targets" << std::endl;
        for (; !ccs_G_ip1_left_uqe.empty(); ++ccs_G_ip1_left_uqe) {
            const auto node_cc_G_i_left = *ccs_G_ip1_left_uqe;

            // relabel every edge that has a matching node with the mapped node above
            for (; !edges_G_ip1_right_upsrc_uqe.empty(); ++edges_G_ip1_right_upsrc_uqe) {
                const auto edge_G_ip1_right_upsrc = *edges_G_ip1_right_upsrc_uqe;

                node_upp_bnd_G_ip1_right_relabel += (last_target_G_ip1 != edge_G_ip1_right_upsrc.v);
                last_target_G_ip1 = edge_G_ip1_right_upsrc.v;

                // relabel here, for (u, v), (n, l) there are three cases
                // 1. v < n, no relabelling for v, output
                // 2. v == n, relabelling for v -> l
                //  2.1 u == l, do not output, selfloop
                //  2.2 u != l, output
                // 3. v > n, skipped too far with the edges, increment node map
                if (edge_G_ip1_right_upsrc.v < node_cc_G_i_left.node) {
                    assert(edge_G_ip1_right_upsrc.u != edge_G_ip1_right_upsrc.v);
                    edges_G_ip1_right_over_left.push(edge_G_ip1_right_upsrc.normalized());
                } else if (edge_G_ip1_right_upsrc.v == node_cc_G_i_left.node) {
                    // skip self-loop
                    if (edge_G_ip1_right_upsrc.u == node_cc_G_i_left.load) continue;
                    const auto relabelled_edge_G_ip1 = edge_t{edge_G_ip1_right_upsrc.u, node_cc_G_i_left.load}.normalized();
                    edges_G_ip1_right_over_left.push(relabelled_edge_G_ip1);
                } else {
                    break;
                }
            }
        }

        std::cout << "  flushing remaining targets" << std::endl;
        for (; !edges_G_ip1_right_upsrc_uqe.empty(); ++edges_G_ip1_right_upsrc_uqe) {
            const auto edge_G_ip1_right_upsrc = *edges_G_ip1_right_upsrc_uqe;
            // flush
            assert(edge_G_ip1_right_upsrc.u != edge_G_ip1_right_upsrc.v);
            edges_G_ip1_right_over_left.push(edge_G_ip1_right_upsrc.normalized());
            // update statistics
            node_upp_bnd_G_ip1_right_relabel += (last_target_G_ip1 != edge_G_ip1_right_upsrc.v);
            last_target_G_ip1 = edge_G_ip1_right_upsrc.v;
        }

        // no longer need only-source-updated edges
        assert(edges_G_ip1_right_upsrc.empty());
        edges_G_ip1_right_upsrc.finish_clear();

        // sort source and target relabelled edges
        std::cout << "  sorting source and target updated edges" << std::endl;
        edges_G_ip1_right_over_left.sort_reuse();

        return node_upp_bnd_G_ip1_right_relabel;
    }

    std::pair<node_t, node_t> process_left(size_t current_level, node_t node_upp_bnd_G_ip1_left) {
        //!! solve left subproblem recursively
        // retrieve edges of left recursive subproblem
        auto & edges_G_ip1_left  = (*sub_edges_levels[current_level + 1]);

        // actually solve the left recursive subproblem
        const auto node_cc_bounds = process(edges_G_ip1_left, node_upp_bnd_G_ip1_left, current_level + 1, true);

        // reset processed left edges
        reset_edges(current_level + 1);

        return node_cc_bounds;
    }

    std::pair<node_t, node_t> process_right(size_t current_level, node_t nodes_upp_bnd_contracted_G_ip1_right, node_cc_sorter_cc_node_less_t& ccs_G_ip1_left_srtd_cc_node_less) {
        auto & edges_G_ip1_right = *sub_edges_levels[current_level];

        if (is_semi_externally_handleable(nodes_upp_bnd_contracted_G_ip1_right, edges_G_ip1_right)) {
            std::cout << "[OPTIMIZATION] Combined Relabelling/Processing in the Basecase of Right Subcall" << std::endl;

            //!! relabel left connected components into right edges without a sorted result
            auto & ccs_G_ip1_left   = *ccs_left [current_level + 1];
            auto & ccs_G_ip1_right  = *ccs_right[current_level + 1];

            foxxll_timer relabelling_timer("Relabelling");

            // relabel sources
            std::cout << "Relabelling Sources (Components Left: " << ccs_G_ip1_left.size() << ") to (Edges: " << edges_G_ip1_right.size() << ")" << std::endl;
            std::cout << "  updating sources" << std::endl;
            edge_sorter_reverse_less_t edges_G_ip1_right_upsrc(edge_reverse_less_cmp(), SORTER_MEM);
            EdgeSorterSourceRelabeller(ccs_G_ip1_left, ccs_G_ip1_left_srtd_cc_node_less, edges_G_ip1_right, edges_G_ip1_right_upsrc);

            // no longer need non-updated edges
            assert(edges_G_ip1_right.empty());
            reset_edges(current_level);

            std::cout << "  sorting source updated edges" << std::endl;
            edges_G_ip1_right_upsrc.sort_reuse();

            // asserts and verification
            assert(sub_edges_levels[current_level + 1]->size() == 0);
            assert(sub_edges_levels[current_level]->size()     == 0);

            // relabel targets
            ccs_G_ip1_left.rewind();
            make_unique_stream<decltype(edges_G_ip1_right_upsrc)> edges_G_ip1_right_upsrc_uqe(edges_G_ip1_right_upsrc, edge_t{MAX_NODE, MAX_NODE});
            PipelinedKruskal semiext_kruskal_algo;
            EdgeSorterTargetRelabeller(ccs_G_ip1_left, edges_G_ip1_right_upsrc_uqe, semiext_kruskal_algo);

            // no longer need only-source-updated edges
            assert(edges_G_ip1_right_upsrc.empty());
            edges_G_ip1_right_upsrc.finish_clear();

            // compute connected components
            semiext_kruskal_algo.process(ccs_G_ip1_right);
            ccs_G_ip1_right.sort_reuse();

            return std::make_pair(semiext_kruskal_algo.get_num_nodes(), semiext_kruskal_algo.get_num_ccs());
        } else {
            //!! relabel left connected components into right edges
            edge_sorter_less_t edges_G_ip1_right_over_left(edge_less_cmp(), SORTER_MEM);
            const node_t node_upp_bnd_G_ip1_relabel = relabel_right_edges(current_level, ccs_G_ip1_left_srtd_cc_node_less, edges_G_ip1_right_over_left);

            //!! solve right subproblem recursively
            const auto [nodes_G_ip1_right, num_ccs_G_ip1_right]
            = process(edges_G_ip1_right_over_left, std::min(node_upp_bnd_G_ip1_relabel, nodes_upp_bnd_contracted_G_ip1_right), current_level + 1, false);

            return std::make_pair(nodes_G_ip1_right, num_ccs_G_ip1_right);
        }
    }

    /**
     *
     * @param current_level
     * @param left
     * @param ccs_G_ip1_left_srtd_cc_node_less  assumed not sorted already
     */
    void merge_left_right_ccs(size_t current_level, bool left, node_cc_sorter_cc_node_less_t& ccs_G_ip1_left_srtd_cc_node_less) {
        //!! merge ccs from left and right recursion
        foxxll_timer merging_timer("Merging");

        auto & ccs_G_ip1_left  = *ccs_left[current_level + 1];
        auto & ccs_G_ip1_right = *ccs_right[current_level + 1];
        auto & ccs_G_i         = get_component_map(left, current_level);

        // merge left and right
        std::cout << "  sorting left connected components by component" << std::endl;
        ccs_G_ip1_left_srtd_cc_node_less.sort_reuse();

        std::cout << "  merging" << std::endl;
        ComponentMerger(ccs_G_ip1_left_srtd_cc_node_less, ccs_G_ip1_right, ccs_G_i);

        // reset old connected components map
        assert(ccs_G_ip1_right.empty());
        assert(ccs_G_ip1_left_srtd_cc_node_less.empty());
        ccs_G_ip1_left_srtd_cc_node_less.finish_clear();
        ccs_G_ip1_left.clear();
        ccs_G_ip1_right.clear();

        // sort
        std::cout << "  sorting merged component map" << std::endl;
        ccs_G_i.sort_reuse();
    }

    /**
     *
     * @param node_contraction_G_i  assumed sorted already
     * @param ccs_contracted_G_i    assumed sorted already
     * @param current_level
     * @param left
     */
    void merge_ccs_over_ccs(node_cc_sorter_cc_node_less_t& node_contraction_G_i, node_cc_sorter_node_cc_less_t& ccs_contracted_G_i, size_t current_level, bool left) {
        //!! merge ccs from left and right recursion
        foxxll_timer merging_timer("Merging");

        // compute merge
        std::cout << "  merging" << std::endl;
        auto & ccs_G_i = get_component_map(left, current_level);
        ComponentMerger(node_contraction_G_i, ccs_contracted_G_i, ccs_G_i);

        // clear used up sorters
        assert(node_contraction_G_i.empty());
        assert(ccs_contracted_G_i.empty());
        node_contraction_G_i.finish_clear();
        ccs_contracted_G_i.finish_clear();

        // sort
        std::cout << "  sorting merged component map" << std::endl;
        ccs_G_i.sort_reuse();
    }

    template <typename InEdges>
    std::pair<node_t, node_t> fully_external(InEdges & in_edges, node_t nodes_upp_bnd, size_t current_level, bool left) {
        const auto nodes_upp_bnd_2 = std::min(nodes_upp_bnd, in_edges.size() * 2);

        std::cout << "External case" << std::endl;
        std::cout << "Level " << current_level << std::endl;
        std::cout << (left ? "Left" : "Right") << std::endl;
        std::cout << "n ≤ " << nodes_upp_bnd_2 << " = min(" << nodes_upp_bnd << ", " << (in_edges.size() * 2) << ")" << std::endl;
        std::cout << "m ≤ " << in_edges.size() << std::endl;

        if (in_edges.empty()) {
            // clear lower recursion level
            validate_depth(current_level + 1);
            ccs_left [current_level + 1]->clear();
            ccs_right[current_level + 1]->clear();

            return std::make_pair(0, 0);
        }

        make_unique_stream<InEdges> in_edges_uqe(in_edges);

        //!! contract edges using star
        bool perform_contraction = policy.perform_contraction(nodes_upp_bnd_2, in_edges.size(), current_level, main_memory_size / (sizeof(node_t) * StreamKruskal::MEMORY_OVERHEAD_FACTOR));
        std::cout << "Ask policy: contract? " << perform_contraction << std::endl;
        node_t nodes_upp_bnd_contracted_G_i_con;

        if (perform_contraction) {
            foxxll::stats *contraction_stats = foxxll::stats::get_instance();
            foxxll::stats_data contraction_stats_begin(*contraction_stats);

            node_cc_sorter_cc_node_less_t node_contraction_G_i(node_component_cc_node_less_cmp(), SORTER_MEM);
            edge_sorter_less_t contracted_edges_G_i(edge_less_cmp(), SORTER_MEM);
            Contraction contraction_algo;

            size_t contraction_goal = policy.contract_number(nodes_upp_bnd_2, in_edges.size(), current_level, main_memory_size / (sizeof(node_t) * StreamKruskal::MEMORY_OVERHEAD_FACTOR));
            std::cout << "Will contract " << contraction_goal << " nodes" << std::endl;
            // NOTE: now dependent on contraction goal; no longer supporting expected contraction ratio
            if (is_semi_externally_handleable(nodes_upp_bnd_2 - contraction_goal) && Contraction::supports_only_map_return()) {
                std::cout << "[OPTIMIZATION] Pipe Contraction into pipelined Kruskal immediately" << std::endl;

                PipelinedKruskal semiext_kruskal_algo;
                contraction_algo.compute_semi_external_contraction(in_edges_uqe, node_contraction_G_i, semiext_kruskal_algo, contraction_goal);
                node_contraction_G_i.sort_reuse();
                const node_t node_contraction_G_i_size = node_contraction_G_i.size();

                node_cc_sorter_node_cc_less_t ccs_contracted_G_i(node_component_node_cc_less_cmp(), SORTER_MEM);
                semiext_kruskal_algo.process(ccs_contracted_G_i);
                ccs_contracted_G_i.sort_reuse();

                // remap node contraction to returned connected components from the base case
                merge_ccs_over_ccs(node_contraction_G_i, ccs_contracted_G_i, current_level, left);

                // clear lower recursion level
                validate_depth(current_level + 1);
                ccs_left [current_level + 1]->clear();
                ccs_right[current_level + 1]->clear();

                return std::make_pair(semiext_kruskal_algo.get_num_nodes() + node_contraction_G_i_size, semiext_kruskal_algo.get_num_ccs());
            }

            contraction_algo.compute_fully_external_contraction(in_edges_uqe, contracted_edges_G_i, node_contraction_G_i, contraction_goal);
            in_edges.clear();
            contracted_edges_G_i.sort_reuse();
            node_contraction_G_i.sort_reuse();

            // NOTE: contraction goal is assumed to be reached (redo for alternative contraction algos)
            const size_t nodes_ub_G_i_con_goal  = nodes_upp_bnd_2 - contraction_goal;
            const size_t nodes_ub_G_i_con_edges = 2 * contracted_edges_G_i.size();
            nodes_upp_bnd_contracted_G_i_con = std::min(nodes_ub_G_i_con_goal, nodes_ub_G_i_con_edges);

            // log
            std::cout << "n' ≤ " << nodes_upp_bnd_contracted_G_i_con << " = min(" << nodes_ub_G_i_con_goal << ", " << nodes_ub_G_i_con_edges << ")" << std::endl;
            std::cout << "m' ≤ " << contracted_edges_G_i.size() << std::endl;

            std::cout << "Contracting: " << (foxxll::stats_data(*contraction_stats) - contraction_stats_begin).get_elapsed_time() << std::endl;

            // if the contraction removed all edges
            if (contracted_edges_G_i.size() == 0 || contracted_edges_G_i.empty()) {
                std::cout << "[OPTIMIZATION] After Contraction Immediate Return" << std::endl;

                //!! merge ccs from left and right recursion
                foxxll_timer merging_timer("Merging");

                // compute merge
                auto & ccs_G_i = get_component_map(left, current_level);
                StreamPusher(node_contraction_G_i, ccs_G_i);

                // clear
                node_contraction_G_i.finish_clear();

                // sort
                std::cout << "  resorting merged component map" << std::endl;
                ccs_G_i.sort_reuse();

                // clear lower recursion level
                validate_depth(current_level + 1);
                ccs_left [current_level + 1]->clear();
                ccs_right[current_level + 1]->clear();

                return std::make_pair(ccs_G_i.size(), ccs_G_i.size());
            }

            // if the contracted edges can be handled semi-externally do it
            if (is_semi_externally_handleable(nodes_upp_bnd_contracted_G_i_con, contracted_edges_G_i)) {
                std::cout << "[OPTIMIZATION] After Contraction Immediate Semi-Ext" << std::endl;

                node_cc_sorter_node_cc_less_t ccs_contracted_G_i(node_component_node_cc_less_cmp(), SORTER_MEM);
                const auto [nodes_G_i, num_ccs_G_i] = semi_external(contracted_edges_G_i, ccs_contracted_G_i);

                // remap node contraction to returned connected components from the base case
                merge_ccs_over_ccs(node_contraction_G_i, ccs_contracted_G_i, current_level, left);

                // clear lower recursion level
                validate_depth(current_level + 1);
                ccs_left [current_level + 1]->clear();
                ccs_right[current_level + 1]->clear();

                return std::make_pair(nodes_G_i, num_ccs_G_i);
            }

            //!! generate subproblems from contracted edges
            // sample the edges
            make_unique_stream<decltype(contracted_edges_G_i)> contracted_edges_G_i_uqe(contracted_edges_G_i, edge_t{MAX_NODE, MAX_NODE});
            std::cout << "Node upper bound before sampling: " << nodes_upp_bnd_contracted_G_i_con << std::endl;
            std::cout << "Number of edges before sampling: " << contracted_edges_G_i_uqe.size() << std::endl;
            int sampling_prob_power = policy.sample_prob_power(nodes_upp_bnd_contracted_G_i_con, contracted_edges_G_i_uqe.size(), current_level, main_memory_size / (sizeof(node_t) * StreamKruskal::MEMORY_OVERHEAD_FACTOR));
            const auto [nodes_upp_bnd_contracted_G_i_sam,
                        nodes_upp_bnd_contracted_G_ip1_left_sam,
                        nodes_upp_bnd_contracted_G_ip1_right_sam,
                        nodes_low_bnd_contracted_G_ip1_common_sam]
	        = sample_edges(contracted_edges_G_i_uqe, current_level, true, sampling_prob_power);

            // compute node upper bounds
            node_t nodes_upp_bnd_contracted_G_i         = std::min(nodes_upp_bnd_contracted_G_i_sam, nodes_upp_bnd_contracted_G_i_con);
            node_t nodes_upp_bnd_contracted_G_ip1_left  = std::min(nodes_upp_bnd_contracted_G_i, nodes_upp_bnd_contracted_G_ip1_left_sam);
            node_t nodes_upp_bnd_contracted_G_ip1_right = std::min(nodes_upp_bnd_contracted_G_i, nodes_upp_bnd_contracted_G_ip1_right_sam);

            // log
            std::cout << "n'' ≤ " << nodes_upp_bnd_contracted_G_ip1_left
                                  << " = min(" << nodes_upp_bnd_contracted_G_i
                                  << ", " << nodes_upp_bnd_contracted_G_ip1_left_sam << ")" << std::endl;
            std::cout << "m'' = " << sub_edges_levels[current_level + 1]->size() << std::endl;

            // contracted G_i no longer needed
            contracted_edges_G_i.finish_clear();

            // if the sampling of the edges reveals that a semi-external run would have already been sufficient, do it
            if (is_semi_externally_handleable(nodes_upp_bnd_contracted_G_i_sam)) {
                std::cout << "[OPTIMIZATION] After Sampling Estimates Fit Semi-Ext" << std::endl;
                auto & edges_G_ip1_left   = *sub_edges_levels[current_level + 1];
                auto & edges_G_ip1_right  = *sub_edges_levels[current_level];

                node_cc_sorter_node_cc_less_t ccs_contracted_G_i(node_component_node_cc_less_cmp(), SORTER_MEM);
                const auto [nodes_G_i, num_ccs_G_i] = semi_external(edges_G_ip1_left, edges_G_ip1_right, ccs_contracted_G_i);
                reset_edges(current_level);
                reset_edges(current_level + 1);

                merge_ccs_over_ccs(node_contraction_G_i, ccs_contracted_G_i, current_level, left);

                // clear lower recursion level
                validate_depth(current_level);
                ccs_left [current_level + 1]->clear();
                ccs_right[current_level + 1]->clear();

                return std::make_pair(nodes_G_i, num_ccs_G_i);
            }

            //!! process left
            // compute connected components of sampled edges
            node_cc_sorter_cc_node_less_t ccs_G_ip1_left_srtd_cc_node_less(node_component_cc_node_less_cmp(), SORTER_MEM);
            const auto [nodes_G_ip1_left, num_ccs_G_ip1_left]
            = process_left(current_level, nodes_upp_bnd_contracted_G_ip1_left);

            // asserts and verification
            assert(sub_edges_levels[current_level + 1]->size() == 0);

            // recompute node bounds
            nodes_upp_bnd_contracted_G_ip1_right
            = std::min(std::min(nodes_upp_bnd_contracted_G_ip1_right,
                       nodes_upp_bnd_contracted_G_ip1_right - nodes_low_bnd_contracted_G_ip1_common_sam + num_ccs_G_ip1_left),
                       nodes_upp_bnd_contracted_G_i - nodes_G_ip1_left + num_ccs_G_ip1_left);

            //!! process right
            // compute connected components of unsampled edges
            const auto [nodes_G_ip1_right, num_ccs_G_ip1_right]
            = process_right(current_level, nodes_upp_bnd_contracted_G_ip1_right, ccs_G_ip1_left_srtd_cc_node_less);
            tlx::unused(nodes_G_ip1_right);

            // asserts and verification
            assert(sub_edges_levels[current_level]->size() == 0);
            assert(ccs_left[current_level + 1]->empty());

            //!! merge ccs from left and right recursion and the contraction
            foxxll::stats *merging_stats = foxxll::stats::get_instance();
            foxxll::stats_data merging_stats_begin(*merging_stats);
            auto & ccs_G_i = get_component_map(left, current_level);

            // prepare components of right subcall
            auto & ccs_G_ip1_right   = *ccs_right[current_level + 1];

            // merge left and right
            node_cc_sorter_node_cc_less_t ccs_G_i_without_stars(node_component_node_cc_less_cmp(), SORTER_MEM);
            ccs_G_ip1_left_srtd_cc_node_less.sort_reuse();
            std::cout << "Merge Component Maps (Left: " << ccs_G_ip1_left_srtd_cc_node_less.size() << ")"
                      << " with (Right: " << ccs_G_ip1_right.size() << ")" << std::endl;
            ComponentMerger(ccs_G_ip1_left_srtd_cc_node_less, ccs_G_ip1_right, ccs_G_i_without_stars);
            ccs_G_i_without_stars.sort_reuse();

            // asserts and verification
            assert(ccs_G_ip1_right.empty());
            assert(ccs_G_ip1_left_srtd_cc_node_less.empty());

            // connected components from left subcall no longer needed
            ccs_G_ip1_left_srtd_cc_node_less.finish_clear();

            // merge contraction map with components of contracted graph
            std::cout << "Merge Component maps (Contraction: " << node_contraction_G_i.size() << ")"
                      << " with (Recursive Left+Right: " << ccs_G_i_without_stars.size() << ")" << std::endl;
            ComponentMerger(node_contraction_G_i, ccs_G_i_without_stars, ccs_G_i);
            node_contraction_G_i.finish_clear();
            ccs_G_i_without_stars.finish_clear();
            std::cout << "  sorting merged component map" << std::endl;
            ccs_G_i.sort_reuse();

            // asserts and verification
            assert(ccs_left[current_level + 1]->empty()); // checks whether the algorithm clears too early

            std::cout << "Merging: " << (foxxll::stats_data(*merging_stats) - merging_stats_begin).get_elapsed_time() << std::endl;

            // clear lower recursion level
            validate_depth(current_level + 1);
            ccs_left [current_level + 1]->clear();
            ccs_right[current_level + 1]->clear();

            return std::make_pair(ccs_G_i.size(), num_ccs_G_ip1_left + num_ccs_G_ip1_right); // TODO improve by counting above
        } else {
            std::cout << "Node upper bound before sampling: " << nodes_upp_bnd << std::endl;
            std::cout << "Number of edges before sampling: " << in_edges_uqe.size() << std::endl;
            int sampling_prob_power = policy.sample_prob_power(nodes_upp_bnd, in_edges_uqe.size(), current_level, main_memory_size / (sizeof(node_t) * StreamKruskal::MEMORY_OVERHEAD_FACTOR));
            const auto [nodes_upp_bnd_G_i_sam,
                        nodes_upp_bnd_G_ip1_left_sam,
                        nodes_upp_bnd_G_ip1_right_sam,
                        nodes_upp_bnd_G_ip1_common_sam]
	            = sample_edges(in_edges_uqe, current_level, false, sampling_prob_power);

            // if the sampling of the edges reveals that a semi-external run would have already been sufficient, do it
            if (is_semi_externally_handleable(nodes_upp_bnd_G_i_sam)) {
                std::cout << "[OPTIMIZATION] After Sampling Estimates Fit Semi-Ext" << std::endl;

                auto & edges_G_ip1_left   = *sub_edges_levels[current_level + 1];
                auto & edges_G_ip1_right  = *sub_edges_levels[current_level];
                auto & ccs_G_i = get_component_map(left, current_level);

                const auto [nodes_G_i, num_ccs_G_i] = semi_external(edges_G_ip1_left, edges_G_ip1_right, ccs_G_i);
                reset_edges(current_level);
                reset_edges(current_level + 1);

                // clear lower recursion level
                validate_depth(current_level + 1);
                ccs_left [current_level + 1]->clear();
                ccs_right[current_level + 1]->clear();

                return std::make_pair(nodes_G_i, num_ccs_G_i);
            }

            //!! process left
            const auto [nodes_G_ip1_left, num_ccs_G_ip1_left]
            = process_left(current_level, std::min(nodes_upp_bnd, nodes_upp_bnd_G_ip1_left_sam));

            // asserts and verification
            assert(sub_edges_levels[current_level + 1]->size() == 0);

            // recompute node bounds
            node_t nodes_upp_bnd_G_ip1_right
            = std::min(std::min(std::min(std::min(nodes_upp_bnd_G_ip1_right_sam,
                     nodes_upp_bnd_G_ip1_right_sam - nodes_upp_bnd_G_ip1_common_sam + num_ccs_G_ip1_left),
                     nodes_upp_bnd_G_i_sam - nodes_G_ip1_left + num_ccs_G_ip1_left),
                       nodes_upp_bnd), nodes_upp_bnd - nodes_G_ip1_left + num_ccs_G_ip1_left);

            //!! process right
            node_cc_sorter_cc_node_less_t ccs_G_ip1_left_srtd_cc_node_less(node_component_cc_node_less_cmp(), SORTER_MEM);
            const auto [nodes_G_ip1_right, num_ccs_G_ip1_right]
            = process_right(current_level, nodes_upp_bnd_G_ip1_right, ccs_G_ip1_left_srtd_cc_node_less);
            tlx::unused(nodes_G_ip1_right);

            // asserts and verification
            assert(sub_edges_levels[current_level]->size()     == 0);
            assert(sub_edges_levels[current_level + 1]->size() == 0);

            //!! merge
            merge_left_right_ccs(current_level, left, ccs_G_ip1_left_srtd_cc_node_less);
            auto & ccs_G_i = get_component_map(left, current_level);

            // clear lower recursion level
            validate_depth(current_level + 1);
            ccs_left [current_level + 1]->clear();
            ccs_right[current_level + 1]->clear();

            return std::make_pair(ccs_G_i.size(), num_ccs_G_ip1_left + num_ccs_G_ip1_right);
        }
    }

    /**
     * @return Pair of upper bound on the number of nodes and its connected component count.
     */
    template <typename InEdges>
    std::pair<node_t, node_t> process(InEdges & in_edges, node_t nodes_upp_bnd, size_t current_level, bool left) {
        std::cout << "Processing" << std::endl;

        if (current_level > latest_max_level) {
            assert(current_level == latest_max_level + 1);
            sub_edges_levels.emplace_back(new edge_sequence_t());
            ccs_left.emplace_back(new node_cc_sorter_node_cc_less_t(node_component_node_cc_less_cmp(), SORTER_MEM));
            ccs_right.emplace_back(new node_cc_sorter_node_cc_less_t(node_component_node_cc_less_cmp(), SORTER_MEM));
            latest_max_level = current_level;
            std::cout << "Current Level is " << current_level << " and number of cc maps is " << ccs_left.size() << " where the last index is " << (ccs_left.size() - 1) << std::endl;
            used_main_memory_size += 2 * SORTER_MEM;
        }

        // base case
        if (is_semi_externally_handleable(nodes_upp_bnd, in_edges)) {
            auto & ccs_G_i = get_component_map(left, current_level);
            assert(ccs_G_i.size() == 0);
            return semi_external(in_edges, ccs_G_i);
        } else {
            assert((left ? *ccs_left[current_level] : *ccs_right[current_level]).size() == 0);
            return fully_external(in_edges, nodes_upp_bnd, current_level, left);
        }
    }

    template <typename InEdges>
    auto sample_edges(InEdges& in_edges, size_t current_level, bool in_place, int sample_prob_power) {
        //!! generate subproblems from incoming edges
        foxxll_timer sampling_timer("Sampling");
        std::cout << "Sampling with probability 1/2^" << sample_prob_power << std::endl;

        PowerOfTwoCoin sample_coin(sample_prob_power);
        auto sample_stream = [&](auto& next_level, auto& this_level,
                                 node_t& count_G_i,
                                 node_t& count_G_ip1_left,
                                 node_t& count_G_ip1_right,
                                 node_t& count_G_ip1_common) {

            edge_t edge_G_i        {MAX_NODE, MAX_NODE};
            edge_t edge_G_ip1_left {MAX_NODE, MAX_NODE};
            edge_t edge_G_ip1_right{MAX_NODE, MAX_NODE};
            bool src_G_i         = true;
            bool src_G_ip1_left  = false;
            bool src_G_ip1_right = false;

            auto increment_counter = [](auto& count, edge_t& curr_edge, edge_t next_edge) {
                count += (curr_edge.u != next_edge.u);
                count += (curr_edge.v != next_edge.v);
                curr_edge = next_edge;
            };

            for (; !in_edges.empty(); ++in_edges) {
                const auto in_edge_uqe = *in_edges;
                // set source flags
                src_G_i = src_G_i || (in_edge_uqe.u != edge_G_i.u);
                src_G_ip1_left = src_G_ip1_left && (in_edge_uqe.u == edge_G_i.u);
                src_G_ip1_right = src_G_ip1_right && (in_edge_uqe.u == edge_G_i.u);

                increment_counter(count_G_i, edge_G_i, in_edge_uqe);
                if (sample_coin(gen)) {
                    src_G_ip1_left = true;
                    next_level.push(in_edge_uqe);
                    increment_counter(count_G_ip1_left, edge_G_ip1_left, in_edge_uqe);
                } else {
                    src_G_ip1_right = true;
                    this_level.push(in_edge_uqe);
                    increment_counter(count_G_ip1_right, edge_G_ip1_right, in_edge_uqe);
                }

                count_G_ip1_common += (src_G_ip1_left && src_G_ip1_right && src_G_i);
                src_G_i = !(src_G_ip1_left && src_G_ip1_right);
            }
        };

        node_t cnt_G_i          = 0;
        node_t cnt_G_ip1_left   = 0;
        node_t cnt_G_ip1_right  = 0;
        node_t cnt_G_ip1_common = 0;
        if (in_place) {
            assert(sub_edges_levels[current_level + 1]->size() == 0);
            reset_edges(current_level);
            sample_stream(*sub_edges_levels[current_level + 1], *sub_edges_levels[current_level], cnt_G_i, cnt_G_ip1_left, cnt_G_ip1_right, cnt_G_ip1_common);
        } else {
            assert(sub_edges_levels[current_level + 1]->size() == 0);
            edge_sequence_t tmp_edges;
            sample_stream(*sub_edges_levels[current_level + 1], tmp_edges, cnt_G_i, cnt_G_ip1_left, cnt_G_ip1_right, cnt_G_ip1_common);
            (*sub_edges_levels[current_level]).swap(tmp_edges);
        }

        sub_edges_levels[current_level    ]->rewind();
        sub_edges_levels[current_level + 1]->rewind();

        std::cout << "Left sample simple node bound: " << cnt_G_ip1_left << std::endl;

        return std::make_tuple(cnt_G_i, cnt_G_ip1_left, cnt_G_ip1_right, cnt_G_ip1_common);
    }

    [[nodiscard]] size_t get_current_depth() const {
        assert(ccs_left.size() == ccs_right.size());
        return ccs_left.size();
    }

    void validate_depth(size_t depth) {
        if (depth >= get_current_depth()) {
            ccs_left.emplace_back(new node_cc_sorter_node_cc_less_t(node_component_node_cc_less_cmp(), SORTER_MEM));
            ccs_right.emplace_back(new node_cc_sorter_node_cc_less_t(node_component_node_cc_less_cmp(), SORTER_MEM));
        }
    }

    void reset_edges(size_t to_reset) {
        sub_edges_levels[to_reset].reset(nullptr);
        sub_edges_levels[to_reset] = std::make_unique<edge_sequence_t>();
    }

    inline auto & get_component_map(bool left, size_t current_level) {
        return (left ? *ccs_left[current_level] : *ccs_right[current_level]);
    }

    [[nodiscard]] bool is_semi_externally_handleable(node_t sub_problem_num_nodes) const {
	    return sub_problem_num_nodes * sizeof(node_t) * StreamKruskal::MEMORY_OVERHEAD_FACTOR <= main_memory_size;
    }

    template <typename InEdges>
    bool is_semi_externally_handleable(node_t sub_problem_num_nodes, InEdges& in_edges) const {
	    return is_semi_externally_handleable(sub_problem_num_nodes) || 2 * sizeof(node_t) * in_edges.size() <= main_memory_size;
    }
};
