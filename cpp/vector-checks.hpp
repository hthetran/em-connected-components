#pragma once

#include "defs.hpp"
#include "util.hpp"

template <class edge_comparator_t>
bool is_sorted(const em_edge_vector& edges, edge_comparator_t cmp) {
	edge_t prev = cmp.min_value();
	for (auto& e: edges) {
		if (!cmp(prev, e)) {
			std::cout << prev << " not cmp " << e << std::endl;
			return false;
		}
		prev = e;
	}
	return true;
}

bool contains_stars_only(const em_edge_vector& E) {
	// assumes E is sorted by source
	em_node_vector right_endpoints;
	for (const edge_t& e: E) {
		right_endpoints.push_back(e.v);
	}
	stxxl::sort(right_endpoints.begin(), right_endpoints.end(),
	            node_lt_ordering(), INTERNAL_SORT_MEM);
	em_node_vector::const_iterator target_it = right_endpoints.begin();
	node_t prev_source = MIN_NODE;
	for (const edge_t& e: E) {
		if (e.u == prev_source) {
			std::cout << prev_source << " " << e << std::endl;
			return false;
		}
		prev_source = e.u;
		while (target_it != right_endpoints.end() && *target_it < e.u) {
			target_it++;
		}
		if (target_it != right_endpoints.end() && *target_it == e.u) {
			if (!e.self_loop()) {
				std::cout << e << " " << *target_it << std::endl;
				return false;
			}
		}
	}
	return true;
}

bool disjoint_sources(const em_edge_vector& E1, const em_edge_vector& E2) {
	// assumes both are sorted by sources
	em_edge_vector::const_iterator e1_it = E1.begin();
	em_edge_vector::const_iterator e2_it = E2.begin();
	while (e1_it != E1.end() && e2_it != E2.end()) {
		while (e1_it != E1.end() && e1_it->u < e2_it->u) {
			e1_it++;
		}
		while (e1_it != E1.end() && e2_it != E2.end() && e1_it->u > e2_it->u) {
			e2_it++;
		}
		if (e1_it != E1.end() && e2_it != E2.end()) {
			if (e1_it->u == e2_it->u) {
				return false;
			}
		}
	}
	return true;
}
