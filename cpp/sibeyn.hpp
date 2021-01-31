#ifndef EM_CC_SIBEYN_H
#define EM_CC_SIBEYN_H

#include <foxxll/mng/read_write_pool.hpp>

#include <stxxl/sort>
#include <stxxl/priority_queue>
#include "defs.hpp"


void tfp(em_edge_vector& tree, em_edge_vector& stars) {
	using pq_type = stxxl::PRIORITY_QUEUE_GENERATOR<edge_t, edge_lt_ordering, INTERNAL_PQ_MEM, MAX_PQ_SIZE>::result;
	using block_type = pq_type::block_type;
	const auto pool_half_mem = PQ_POOL_MEM / 2 / block_type::raw_size;
	foxxll::read_write_pool<block_type> pool(pool_half_mem, pool_half_mem);
	pq_type pq(pool);

	for (edge_t& e: stars) {
		pq.push(e);
	}

	stxxl::sort(tree.begin(), tree.end(), edge_gt_ordering(), INTERNAL_SORT_MEM);
	edge_t msg;
	node_t current_node = MAX_NODE;
	node_t current_root = MAX_NODE;

	for (edge_t& e: tree) {
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
			}
		}
		edge_t assignment(e.v, current_root);
		stars.push_back(assignment);
		assert(assignment.u < assignment.v);
		pq.push(assignment); // inform target of this root
	}
}


void run_sibeyn_old(size_t contraction_goal, em_edge_vector& input, em_edge_vector& output, em_edge_vector& leftover) {
	using pq_type = stxxl::PRIORITY_QUEUE_GENERATOR<edge_t, edge_gt_lt_ordering, INTERNAL_PQ_MEM, MAX_PQ_SIZE>::result;
	using block_type = pq_type::block_type;
	const auto pool_half_mem = PQ_POOL_MEM / 2 / block_type::raw_size;
	foxxll::read_write_pool<block_type> pool(pool_half_mem, pool_half_mem);
	pq_type pq(pool);
	while (!input.empty()) {
		const edge_t& e = input.back();
		assert(e.u < e.v);
		pq.push(e);
		input.pop_back();
	}

	edge_t previous_edge = edge_t(MIN_NODE, MAX_NODE); // changes for every pop
	node_t original_u = previous_edge.u; // used to detect when we start processing a new node
	node_t furthest_v = previous_edge.v;
	unsigned long processed_edges = 0;
	size_t contracted_nodes = 0;
	while (!pq.empty()) {
		edge_t current_edge = pq.top();
		pq.pop();
		processed_edges++;

		assert(current_edge.u < current_edge.v);
		if (current_edge == previous_edge) {
			continue;
		}

		if (current_edge.u == original_u) {
			edge_t new_edge(current_edge.v, furthest_v);
			assert(new_edge.v > new_edge.u);
			pq.push(new_edge);
		} else {
			assert(original_u < current_edge.u);
			contracted_nodes++;
			if (contracted_nodes > contraction_goal) {
				pq.push(current_edge);
				break;
			}
			output.push_back(current_edge);
			original_u = current_edge.u;
			furthest_v = current_edge.v;
		}
		previous_edge = current_edge;
	}

	edge_t prev(MIN_NODE, MAX_NODE);
	while (!pq.empty()) {
		edge_t e = pq.top();
		assert(e.u <= e.v);
		pq.pop();
		if (e != prev) {
			leftover.push_back(e);
			prev = e;
		}
	}
}

void run_sibeyn(size_t contraction_goal, const em_edge_vector& input, em_edge_vector& output, em_edge_vector& leftover) {
	// this version assumes that the input is sorted
	using pq_type = stxxl::PRIORITY_QUEUE_GENERATOR<edge_t, edge_gt_lt_ordering, INTERNAL_PQ_MEM, MAX_PQ_SIZE>::result;
	using block_type = pq_type::block_type;
	const unsigned int pool_mem = (16*1024*1024)/block_type::raw_size; // TODO: figure out a reasonable pool size
	foxxll::read_write_pool<block_type> pool(pool_mem/2, pool_mem/2);
	pq_type pq(pool);
	// only want to contract the first contraction_goal sources; find and insert all edges from them
	node_t prev_source=MIN_NODE;
	size_t unique_sources=0;
	node_t uninserted_node=MAX_NODE;
	auto input_it = input.begin();
	while (input_it != input.end()) {
		const edge_t& e = *input_it;
		if (prev_source != e.u) {
			unique_sources++;
			prev_source = e.u;
			if (unique_sources > contraction_goal) {
				uninserted_node = e.u;
				break;
			}
		}
		assert(e.u < e.v);
		assert(unique_sources <= contraction_goal);
		pq.push(e);
		input_it++;
	}
	em_edge_vector overshot_pq;

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
		assert(current_edge.u < uninserted_node);
		if (current_edge == prev) {
			continue;
		}

		if (current_edge.u == original_u) {
			edge_t new_edge(current_edge.v, furthest_v);
			assert(new_edge.u < new_edge.v);
			if (new_edge.u < uninserted_node) {
				pq.push(new_edge);
			} else {
				overshot_pq.push_back(new_edge);
			}
		} else {
			assert(original_u < current_edge.u);
			contracted_nodes++;
			if (contracted_nodes > contraction_goal) {
				pq.push(current_edge);
				break;
			}
			output.push_back(current_edge);
			original_u = current_edge.u;
			furthest_v = current_edge.v;
		}
		prev = current_edge;
	}

	prev = edge_t(MIN_NODE, MAX_NODE);
	while (!pq.empty()) {
		edge_t e = pq.top();
		assert(e.u <= e.v);
		pq.pop();
		if (e != prev) {
			leftover.push_back(e);
			prev = e;
		}
	}

	auto comp = edge_lt_ordering();
	stxxl::sort(overshot_pq.begin(), overshot_pq.end(), comp, INTERNAL_SORT_MEM);
	edge_t next;
	auto overshot_it = overshot_pq.begin();
	while (input_it != input.end() && overshot_it != overshot_pq.end()) {
		if (comp(*input_it, *overshot_it)) {
			next = *input_it;
			input_it++;
		} else {
			next = *overshot_it;
			overshot_it++;
		}
		if (next != prev) {
			prev = next;
			leftover.push_back(next);
		}
	}
	while (input_it != input.end()) {
		if (*input_it != prev) {
			prev = *input_it;
			leftover.push_back(*input_it);
		}
		input_it++;
	}
	while (overshot_it != overshot_pq.end()) {
		if (*overshot_it != prev) {
			prev = *overshot_it;
			leftover.push_back(*overshot_it);
		}
		overshot_it++;
	}
}

#endif
