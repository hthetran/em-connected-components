/*
 * BoruvkaContraction.h
 *
 * Copyright (C) 2019 - 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#pragma once

#include <stxxl/priority_queue>
#include <stxxl/sequence>
#include <stxxl/sorter>
#include "../../defs.hpp"
#include "../hungdefs.hpp"
#include "../relabelling/EdgeSorterRelabeller.h"
#include "../transforms/make_unique_stream.h"
#include "../transforms/make_consecutively_filtered_stream.h"
#include "../utils/StreamMerge.h"
#include "../utils/StreamFilter.h"
#include "../utils/StreamPusher.h"
#include "../basecase/PipelinedKruskal.h"

namespace BoruvkaContraction_details {

struct flagged_edge_t {
    node_t u;
    node_t v;
    bool is_successor;

    flagged_edge_t() = default;
    flagged_edge_t(node_t u_, node_t v_, bool is_successor_) : u(u_), v(v_), is_successor(is_successor_) { }
};

struct repr_msg_t {
    node_t target;
    node_t repr;
    edge_t prio;

    repr_msg_t() = default;
    repr_msg_t(node_t target_, node_t repr_, edge_t prio_) : target(target_), repr(repr_), prio(prio_) { }
};

struct repr_msg_pq_cmp {
    const edge_greater_ordered_cmp prio_cmp = edge_greater_ordered_cmp();

    bool operator() (const repr_msg_t & a, const repr_msg_t & b) const {
        return prio_cmp(a.prio, b.prio);
    }

    [[nodiscard]] repr_msg_t min_value() const {
        return {MAX_NODE, MAX_NODE, {MAX_NODE, MAX_NODE}};
    }

    [[nodiscard]] repr_msg_t max_value() const {
        return {0ul, 0ul, {0ul, 0ul}};
    }
};

template <typename EdgesIn>
class IndexedEdgeSequenceStream {
public:
    using value_type  = ranked_edge_t;
    explicit IndexedEdgeSequenceStream(EdgesIn & in_edges_) : in_edges(in_edges_) { }

    [[nodiscard]] bool empty() const {
        return in_edges.empty();
    }

    [[nodiscard]] size_t size() const {
        return in_edges.size();
    }

    value_type operator * () const {
        const auto in_edge = *in_edges;
        return value_type{in_edge.u, in_edge.v, index};
    }

    IndexedEdgeSequenceStream& operator ++ () {
        ++index;
        ++in_edges;
        return *this;
    }
private:
    EdgesIn& in_edges;
    size_t index = 0;
};
}

/**
 * Follows the logic in the subsection 2.2.1 in
 * On external-memory MST, SSSP and multi-way planar graph separation by L. Arge, G. S. Brodal, L. Toma.
 *
 * The implementation assumes that the incoming edges are sorted.
 */
class BoruvkaContraction {
    using edge_sorter_reverse_less_t    = stxxl::sorter<edge_t, edge_reverse_less_cmp>;
    using node_cc_sorter_node_cc_less_t = stxxl::sorter<node_component_t, node_component_node_cc_less_cmp>;

public:
    BoruvkaContraction() = default;

    template <typename EdgesIn, typename ComponentsOut>
    void compute_semi_external_contraction(EdgesIn& in_edges, ComponentsOut& node_mapping, PipelinedKruskal& kruskal, size_t) {
        tlx::unused(in_edges);
        tlx::unused(node_mapping);
        tlx::unused(kruskal);
        assert(false);
    }

    template <typename EdgesIn, typename EdgesOut, typename ComponentsOut>
    void compute_fully_external_contraction(EdgesIn& in_edges, EdgesOut& out_edges, ComponentsOut& comp_labels, size_t) {
        assert(!in_edges.empty());

        // double the edges and sort them lexicographically
        stxxl::sorter<edge_t, edge_less_cmp> bidir_edges(edge_less_cmp(), SORTER_MEM);
        for (; !in_edges.empty(); ++in_edges) {
            const auto edge = *in_edges;
            bidir_edges.push(edge_t{edge.u, edge.v});
            bidir_edges.push(edge_t{edge.v, edge.u});
        }
        bidir_edges.sort_reuse();

        // retrieve minimum incident neighbour
        node_t last_src = MAX_NODE;
        stxxl::sorter<edge_t, edge_less_ordered_cmp> phase_edges(edge_less_ordered_cmp(), SORTER_MEM);
        for (; !bidir_edges.empty(); ++bidir_edges) {
            const auto dir_edge = *bidir_edges;
            const auto src = dir_edge.u;
            if (src == last_src) continue;

            phase_edges.push(edge_t{dir_edge.v, src});
            last_src = src;
        }
        phase_edges.sort_reuse();

        // if an edge appeared twice, then we can root a tree on one of the endpoints, emit those and sort
        auto seq_eq = [](const edge_t & a, const edge_t & b) {
            // faster than std::minmax
            const auto a1 = std::min(a.u, a.v);
            const auto a2 = std::max(a.u, a.v);
            const auto b1 = std::min(b.u, b.v);
            const auto b2 = std::max(b.u, b.v);

            return a1 == b1 && a2 == b2;
        };

        stxxl::sorter<edge_t, edge_less_cmp> cycleless_edges_lex(edge_less_cmp(), SORTER_MEM);
        stxxl::sorter<node_t, node_less_cmp> tree_roots(node_less_cmp(), SORTER_MEM);
        edge_t last_edge{MAX_NODE, MAX_NODE};
        for (; !phase_edges.empty(); ++phase_edges) {
            const auto edge = *phase_edges;
            if (seq_eq(edge, last_edge))
                tree_roots.push(edge.v);
            else
                cycleless_edges_lex.push(edge);

            last_edge = edge;
        }
        tree_roots.sort_reuse();

        // the number of vertices in the contracted graph is the number of computed pseudo-trees
        node_upper_bound = tree_roots.size();
        cycleless_edges_lex.sort_reuse();
        assert(!tree_roots.empty());

        // construct sorted list of edges where after (u, v) we store all edges incident to v,
        // we send all incident edges to the position emitted by the edge (u, v), by a parallel scan
        // the resulting edge list is called L and is the result of merging the two sequences (L contains each edge twice)
        using incident_edge_pos        = loaded_node_t<node_t>;
        using incident_edge_pos_sorter = stxxl::sorter<incident_edge_pos, node_pos_less_cmp>;
        incident_edge_pos_sorter inc_ep_sorter(node_pos_less_cmp(), SORTER_MEM);

        using cycleless_edge_stream_type = make_consecutively_filtered_stream<decltype(phase_edges), decltype(seq_eq)>;
        phase_edges.rewind();
        cycleless_edge_stream_type cycleless_edgestream(phase_edges, seq_eq);

        for (size_t pos = 0; !cycleless_edgestream.empty(); ++cycleless_edgestream, pos++) {
            const auto edge = *cycleless_edgestream;
            inc_ep_sorter.push(incident_edge_pos{edge.v, pos});
        }
        inc_ep_sorter.sort_reuse();

        stxxl::sorter<ranked_edge_t, ranked_edge_load_edge_less_cmp> incoming_incident_edges(ranked_edge_load_edge_less_cmp(), SORTER_MEM);
        while (!inc_ep_sorter.empty()) {
            const auto top_target_msg = *inc_ep_sorter;
            const auto top_target_node = top_target_msg.node;
            const auto top_target_pos = top_target_msg.load;

            if (cycleless_edges_lex.empty())
                break;
            else {
                edge_t top_lex_edge{};
                for (top_lex_edge = *cycleless_edges_lex; top_lex_edge.u < top_target_node && !cycleless_edges_lex.empty();
                     ++cycleless_edges_lex, top_lex_edge = (cycleless_edges_lex.empty() ? top_lex_edge : *cycleless_edges_lex));

                if (top_lex_edge.u < top_target_node)
                    break;

                if (top_lex_edge.u > top_target_node)
                    ++inc_ep_sorter;
                else {
                    for (; !cycleless_edges_lex.empty(); ++cycleless_edges_lex) {
                        const auto edge = *cycleless_edges_lex;
                        if (edge.u > top_target_node)
                            break;
                        else {
                            std::cout << "ttp" << top_target_pos << std::endl;
                            incoming_incident_edges.push(ranked_edge_t{edge.u, edge.v, top_target_pos});
                        }
                    }
                }
            }
        }
        incoming_incident_edges.sort_reuse();

        // index each edge in a pipelined way
        auto& original_edges = cycleless_edgestream;
        original_edges.rewind();
        BoruvkaContraction_details::IndexedEdgeSequenceStream ranked_original_edges(original_edges);

        // TODO put all of this stuff into BoruvkaContraction_details
        struct Selector {
            [[nodiscard]] bool select(const ranked_edge_t & a, const ranked_edge_t & b) const {
                const size_t a_rank = 2 * a.load;
                const size_t b_rank = 2 * b.load + 1;
                return a_rank > b_rank;
            }
        };
        struct Extractor {
            using value_type = BoruvkaContraction_details::flagged_edge_t;
            [[nodiscard]] value_type extract(const ranked_edge_t & a, const bool & flag) const {
                return value_type{a.u, a.v, flag};
            }
        };

        const Selector select;
        const Extractor extract;
        using PQOrderedEdges = TwoWayStreamMerge<decltype(ranked_original_edges), decltype(incoming_incident_edges), Selector, Extractor>;
        PQOrderedEdges L(ranked_original_edges, incoming_incident_edges, select, extract);

        // setup external-memory priority queue for the messages that inform nodes of their representative,
        // the nodes are inserted with priority of the incoming edge, additionally we need to inform the node of the
        // representative for the next phase
        using ReprMsg         = BoruvkaContraction_details::repr_msg_t;
        using ReprMsgCmp      = BoruvkaContraction_details::repr_msg_pq_cmp;
        using repr_pq_type    = typename stxxl::PRIORITY_QUEUE_GENERATOR<ReprMsg, ReprMsgCmp, INTERNAL_PQ_MEM, 1>::result;
        using repr_block_type = typename repr_pq_type::block_type;
        const auto PQ_POOL_MEM_HALF_BLOCKS = (PQ_POOL_MEM / 2) / repr_block_type::raw_size;
        foxxll::read_write_pool<repr_block_type> repr_pool(PQ_POOL_MEM_HALF_BLOCKS, PQ_POOL_MEM_HALF_BLOCKS);
        repr_pq_type repr_pq(repr_pool);

        // initialize external-memory priority queue with immediate successors of the roots
        cycleless_edges_lex.rewind();
        for (; !tree_roots.empty(); ++tree_roots) {
            const auto root = *tree_roots;
            comp_labels.push(node_component_t{root, root}); // add representatives of roots

            while ((*cycleless_edges_lex).u < root)
                ++cycleless_edges_lex;

            assert((*cycleless_edges_lex).u == root); // must be present in the edge-list

            for (; !cycleless_edges_lex.empty(); ++cycleless_edges_lex) {
                const auto edge = *cycleless_edges_lex;
                if (edge.u > root) break;
                repr_pq.push(ReprMsg{edge.v, root, edge});
            }
        }
        cycleless_edges_lex.finish_clear();

        // process L
        for (; !L.empty(); ) {
            // retrieve non successor edges
            #ifndef NDEBUG
            const auto edge = *L;
            #endif
            ++L;
            assert(!edge.is_successor);

            // pop representative message
            const auto repr_msg = repr_pq.top();
            repr_pq.pop();
            assert(edge.u == repr_msg.prio.u && edge.v == repr_msg.prio.v);

            // add representative of target node
            comp_labels.push(node_component_t{repr_msg.target, repr_msg.repr});

            for (; !L.empty(); ++L) {
                const auto inc_edge = *L;
                if (!inc_edge.is_successor)
                    break;

                repr_pq.push(ReprMsg{inc_edge.v, repr_msg.repr, {inc_edge.u, inc_edge.v}});
            }
        }

        // asserts
        assert(repr_pq.empty());

        // sort computed node component entries
        comp_labels.sort_reuse();

        // relabel sources
        in_edges.rewind();
        edge_sorter_reverse_less_t src_updated_edges(edge_reverse_less_cmp(), SORTER_MEM);
        EdgeSorterSourceRelabeller(comp_labels, in_edges, src_updated_edges);
        src_updated_edges.sort_reuse();

        // relabel targets
        comp_labels.rewind();
        make_unique_stream<decltype(src_updated_edges)> src_updated_edges_uqe(src_updated_edges, edge_t{MAX_NODE, MAX_NODE});
        EdgeSorterTargetRelabeller(comp_labels, src_updated_edges_uqe, out_edges);

        // prepare
        comp_labels.rewind();
    }

    [[nodiscard]] node_t get_node_upper_bound() const {
        return node_upper_bound;
    }

    static bool supports_only_map_return() {
        return false;
    }

    static double get_expected_contraction_ratio_upper_bound() {
        return 0.5;
    }

private:
    node_t node_upper_bound = 0;
};
