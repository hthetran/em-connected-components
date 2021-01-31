/*
 * Sibeyn.hpp
 *
 * Copyright (C) 2020 David Hammer
 */

#pragma once

#include <foxxll/mng/read_write_pool.hpp>
#include <stxxl/priority_queue>

#include "../../defs.hpp"
#include "../../stream-checks.hpp"
#include "../../stream-utils.hpp"
#include "../containers/EdgeStream.h"
#include "../containers/less_alloc_forward_sequence.h"
#include "../transforms/make_unique_stream.h"
#include "../basecase/PipelinedKruskal.h"
#include "../utils/StreamPusher.h"

// note: this could be generalized significantly
template <typename pq_type>
class StreamMessagesForSource {
public:
	using value_type = node_t;
private:
	pq_type& pq;
	value_type source;
	value_type current;
public:
	StreamMessagesForSource(pq_type& pq_, const value_type source_) : pq(pq_), source(source_) {
		if (!empty()) {
			current = pq.top().v;
		}
	}

	[[nodiscard]] bool empty() const {
		return pq.empty() || pq.top().u != source;
	}

	const value_type& operator* () const {
		return current;
	}

	StreamMessagesForSource& operator++ () {
		pq.pop();
		if (!empty()) {
			current = pq.top().v;
		}
		return *this;
	}
};

class SibeynContraction {
public:
	template <typename EdgesIn, typename ComponentsOut>
	void compute_semi_external_contraction(EdgesIn& in_edges, ComponentsOut& star_mapping, PipelinedKruskal& kruskal, size_t contraction_goal) {
		EdgeStream tree_edges;
		run_sibeyn_tuned(in_edges, contraction_goal, tree_edges, kruskal);
		tree_edges.consume();
		StreamEdgesOrientReverse reversed_tree_edges(tree_edges);
		tfp(reversed_tree_edges, star_mapping);
	}

	template <typename EdgesIn, typename EdgesOut, typename ComponentsOut>
	void compute_fully_external_contraction(EdgesIn& in_edges, EdgesOut& contracted_edges, ComponentsOut& star_mapping, size_t contraction_goal) {
		EdgeStream tree_edges;
		run_sibeyn_tuned(in_edges, contraction_goal, tree_edges, contracted_edges);
		// note: contracted edges will be sorted outside by manager
		tree_edges.consume();
		StreamEdgesOrientReverse reversed_tree_edges(tree_edges);
		tfp(reversed_tree_edges, star_mapping);
	}

	static bool supports_only_map_return() {
		return true;
	}

	static double get_expected_contraction_ratio_upper_bound() {
		// TODO: use contraction goal here
		return 0.5;
	}
};

template <typename edge_stream1, typename edge_stream2, typename edge_stream3>
void run_sibeyn(edge_stream1& input_edges, size_t contract_num, edge_stream2& output_tree, edge_stream3& output_edges) {
	// this version assumes that the input is sorted
	assert(is_sorted(input_edges, edge_lt_ordering()));
	using pq_type = stxxl::PRIORITY_QUEUE_GENERATOR<edge_t, edge_gt_lt_ordering, INTERNAL_PQ_MEM, MAX_PQ_SIZE>::result;
	using block_type = pq_type::block_type;
	const auto pool_half_mem = PQ_POOL_MEM / 2 / block_type::raw_size;
	foxxll::read_write_pool<block_type> pool(pool_half_mem, pool_half_mem);
	pq_type pq(pool);
	// only want to contract the first contraction_goal sources; find and insert all edges from them
	node_t uninserted_node_cutoff = MAX_NODE;
	{
		node_t prev_source = MIN_NODE;
		size_t unique_sources = 0;
		for (; !input_edges.empty(); ++input_edges) {
			const edge_t e = *input_edges;
			if (prev_source != e.u) {
				unique_sources++;
				prev_source = e.u;
				if (unique_sources > contract_num) {
					uninserted_node_cutoff = e.u;
					break;
				}
			}
			pq.push(e);
		}
	}
	// now, *input_edges should point to the first edge _not_ inserted in pq

	using edge_sorter = stxxl::sorter<edge_t, edge_lt_ordering>;
	edge_sorter overshot_pq(edge_lt_ordering(), SORTER_MEM);

	edge_t prev = edge_t(MIN_NODE, MAX_NODE); // changes for every pop
	node_t original_u = prev.u; // used to detect when we start processing a new node
	node_t furthest_v = prev.v;
	unsigned long processed_edges = 0;
	size_t contracted_nodes = 0;
	while (!pq.empty()) {
		edge_t current_edge = pq.top();
		pq.pop();
		processed_edges++;

		assert(current_edge.u < current_edge.v);
		assert(current_edge.u < uninserted_node_cutoff);
		if (current_edge == prev) {
			continue;
		}

		if (current_edge.u == original_u) {
			edge_t new_edge(current_edge.v, furthest_v);
			assert(new_edge.u < new_edge.v);
			if (new_edge.u < uninserted_node_cutoff) {
				pq.push(new_edge);
			} else {
				overshot_pq.push(new_edge);
			}
		} else {
			assert(original_u < current_edge.u);
			contracted_nodes++;
			if (contracted_nodes > contract_num) {
				pq.push(current_edge);
				break;
			}
			output_tree.push(current_edge);
			original_u = current_edge.u;
			furthest_v = current_edge.v;
		}
		prev = current_edge;
	}
	overshot_pq.sort();

	prev = edge_t(MIN_NODE, MAX_NODE);
	while (!pq.empty()) {
		edge_t e = pq.top();
		assert(e.u <= e.v);
		pq.pop();
		if (e != prev) {
			output_edges.push(e);
			prev = e;
		}
	}

	make_unique_stream<edge_sorter> unique_overshot_pq(overshot_pq);
	assert(is_sorted(unique_overshot_pq, edge_lt_ordering()));
	StreamMergerUnique merged(input_edges, unique_overshot_pq, edge_lt_ordering());
	StreamPusher(merged, output_edges);
}

template <typename edge_stream1, typename edge_stream2, typename edge_stream3>
void run_sibeyn_tuned(edge_stream1& input_edges, size_t contract_num, edge_stream2& output_tree, edge_stream3& output_edges) {
	// this version assumes that the input is sorted
	assert(is_sorted(input_edges, edge_lt_ordering()));
	using pq_type = stxxl::PRIORITY_QUEUE_GENERATOR<edge_t, edge_gt_lt_ordering, INTERNAL_PQ_MEM, MAX_PQ_SIZE>::result;
	using block_type = pq_type::block_type;
	const auto pool_half_mem = (PQ_POOL_MEM / 2) / block_type::raw_size;
	foxxll::read_write_pool<block_type> pool(pool_half_mem, pool_half_mem);
	pq_type pq(pool);
	size_t contracted_nodes = 0;

    stxxl::less_alloc_forward_sequence<node_t> neighbors_input;
	while (!pq.empty() || !input_edges.empty()) {
		// get next source
		const node_t u_input = (!input_edges.empty() ? (*input_edges).u : MAX_NODE);
		const node_t u_message = (!pq.empty() ? pq.top().u : MAX_NODE);
		const node_t u_current = std::min(u_input, u_message);
		assert(u_current < MAX_NODE);

		// find furthest neighbor to contract to
		// first, gather all neighbors for u_current in input
		while (!input_edges.empty() && (*input_edges).u == u_current) {
			neighbors_input.push_back((*input_edges).v);
			++input_edges;
		}
		const node_t contraction_candidate_input = (!neighbors_input.empty() ? neighbors_input.back() : MIN_NODE);

		// then look at pq
		StreamMessagesForSource messages_in_pq(pq, u_current);
		make_unique_stream unique_messages_in_pq(messages_in_pq);

		// due to PQ order, first message has highest v
		const node_t contraction_candidate_pq = (!unique_messages_in_pq.empty() ? *unique_messages_in_pq : MIN_NODE);

		// and take furthest neighbor found
		const node_t v_contraction_target = std::max(contraction_candidate_input, contraction_candidate_pq);

		// output the tree edge
		output_tree.push(edge_t{u_current, v_contraction_target});

		// and send signals
		// first from original edges
		for (auto input_neighbor_stream = neighbors_input.get_stream(); !input_neighbor_stream.empty(); ++input_neighbor_stream) {
			const edge_t msg = edge_t{*input_neighbor_stream, v_contraction_target};
			assert(msg.u <= msg.v);
			assert(u_current < msg.u);
			if (!msg.self_loop()) {
				pq.push(msg);
			}
		}
		// second from signals
		for (; !unique_messages_in_pq.empty(); ++unique_messages_in_pq) {
			const edge_t msg = edge_t{*unique_messages_in_pq, v_contraction_target};
			assert(msg.u <= msg.v);
			assert(u_current < msg.u);
			if (!msg.self_loop()) {
				pq.push(msg);
			}
		}

		++contracted_nodes;
		if (contracted_nodes == contract_num) {
		    neighbors_input.reset();
			break;
		}

		neighbors_input.reset();
	}

	// warning: not deduplicating between edges and signals (not necessary for Kruskal)
	for (; !input_edges.empty(); ++input_edges) {
		output_edges.push(*input_edges);
	}
	edge_t prev = edge_t(MIN_NODE, MAX_NODE);
	edge_t current_edge = prev;
	while (!pq.empty()) {
		current_edge = pq.top();
		pq.pop();
		if (current_edge == prev) {
			continue;
		}
		output_edges.push(current_edge);
		prev = current_edge;
	}
}

template <typename edge_stream1, typename edge_stream2>
void tfp(edge_stream1& input_tree, edge_stream2& output_stars) {
	// assumes tree edges are oriented opposite from in sibeyn (e.g. larger-to-smaller)
	// this version doesn't take existing star edges (e.g. from base case)
	using pq_type = stxxl::PRIORITY_QUEUE_GENERATOR<edge_t, edge_lt_ordering, INTERNAL_PQ_MEM, MAX_PQ_SIZE>::result;
	using block_type = pq_type::block_type;
	const auto pool_half_mem = PQ_POOL_MEM / 2 / block_type::raw_size;
	foxxll::read_write_pool<block_type> pool(pool_half_mem, pool_half_mem);
	pq_type pq(pool);

	using edge_reverse_sorter = stxxl::sorter<edge_t, edge_gt_ordering>;
	// going "backwards" through tree edges
	// TODO: probably handle this outside instead of flushing around
	edge_reverse_sorter tree_reversed(edge_gt_ordering(), SORTER_MEM);
	flush(input_tree, tree_reversed);
	tree_reversed.sort();
	using edge_sorter = stxxl::sorter<edge_t, edge_lt_ordering>;
	// want to output sorted edges
	edge_sorter star_sorter(edge_lt_ordering(), SORTER_MEM);
	edge_t msg;
	node_t current_node = MAX_NODE;
	node_t current_root = MAX_NODE;
	for (; !tree_reversed.empty(); ++tree_reversed) {
		const edge_t e = *tree_reversed;
		if (e.u != current_node) {
			current_node = e.u;
			current_root = e.u;
			while (!pq.empty() && (msg=pq.top()).u > e.u) {
				// edges not meeting anyone hit root; would assert msg.u<msg.v if not for Kruskal
				pq.pop();
			}
			if (!pq.empty() && (msg=pq.top()).u == e.u) {
				// edge sent here; would assert msg.u<msg.v if not for Kruskal
				current_root = msg.v;
				pq.pop();
			}
			if (current_node == current_root) {
				// new root found
				// TODO: decide globally how to handle self-loops for roots
				// (could output this one)
				output_stars.push(node_component_t{current_node, current_node});
			}
		}
		edge_t assignment(e.v, current_root);
		star_sorter.push(assignment);
		assert(assignment.u < assignment.v);
		pq.push(assignment); // inform target of this root
	}
	star_sorter.sort();
	for (; !star_sorter.empty(); ++star_sorter) {
	    const auto star_edge = *star_sorter;
	    output_stars.push(node_component_t{star_edge.u, star_edge.v});
	}
}

template <typename edge_stream1, typename edge_stream2, typename edge_stream3>
void tfp_after_basecase(edge_stream1& input_tree, edge_stream2& input_stars, edge_stream3& output_stars) {
	// assumes tree edges are oriented opposite from in sibeyn (e.g. larger-to-smaller)
	using pq_type = stxxl::PRIORITY_QUEUE_GENERATOR<edge_t, edge_lt_ordering, INTERNAL_PQ_MEM, MAX_PQ_SIZE>::result;
	using block_type = pq_type::block_type;
	const auto pool_half_mem = PQ_POOL_MEM / 2 / block_type::raw_size;
	foxxll::read_write_pool<block_type> pool(pool_half_mem, pool_half_mem);
	pq_type pq(pool);
	for (; !input_stars.empty(); ++input_stars) {
		pq.push(*input_stars);
		output_stars.push(*input_stars);
	}

	using edge_reverse_sorter = stxxl::sorter<edge_t, edge_gt_ordering>;
	// going "backwards" through tree edges
	// TODO: probably handle this outside instead of flushing around
	edge_reverse_sorter tree_reversed(edge_gt_ordering(), SORTER_MEM);
	flush(input_tree, tree_reversed);
	tree_reversed.sort();
	using edge_sorter = stxxl::sorter<edge_t, edge_lt_ordering>;
	// want to output sorted edges
	edge_sorter star_sorter(edge_lt_ordering(), SORTER_MEM);
	edge_t msg;
	node_t current_node = MAX_NODE;
	node_t current_root = MAX_NODE;
	for (; !tree_reversed.empty(); ++tree_reversed) {
		const edge_t e = *tree_reversed;
		if (e.u != current_node) {
			current_node = e.u;
			current_root = e.u;
			while (!pq.empty() && (msg=pq.top()).u > e.u) {
				// edges not meeting anyone hit root; would assert msg.u<msg.v if not for Kruskal
				pq.pop();
			}
			if (!pq.empty() && (msg=pq.top()).u == e.u) {
				// edge sent here; would assert msg.u<msg.v if not for Kruskal
				current_root = msg.v;
				pq.pop();
			}
			if (current_node == current_root) {
				// new root found
				// TODO: decide globally how to handle self-loops for roots
				// (currently, this will be output)
			}
		}
		edge_t assignment(e.v, current_root);
		output_stars.push(assignment);
		assert(assignment.u < assignment.v);
		pq.push(assignment); // inform target of this root
	}
}
