#pragma once

#include <stxxl/sorter>

template <typename edge_stream_1_t, typename edge_stream_2_t>
bool disjoint_sources(edge_stream_1_t& stream1, edge_stream_2_t& stream2) {
	while (!stream1.empty() && !stream2.empty()) {
		if ((*stream1).u == (*stream2).u) {
			stream1.rewind();
			stream2.rewind();
			return false;
		} else if (*stream1 <= *stream2) {
			++stream1;
		} else {
			++stream2;
		}
	}
	stream1.rewind();
	stream2.rewind();
	return true;
}

template <typename edge_stream_t>
bool only_stars(edge_stream_t& edges) {
	// assumes edges is a sorted stream already
	using node_sorter = stxxl::sorter<node_t, node_lt_ordering>;
	node_sorter right_endpoints(node_lt_ordering(), SORTER_MEM);
	for (; !edges.empty(); ++edges) {
		right_endpoints.push((*edges).v);
	}
	right_endpoints.sort();
	edges.rewind();
	node_t prev_source = MIN_NODE;
	for (; !edges.empty(); ++edges) {
		const edge_t e = *edges;
		if (e.u == prev_source) {
			edges.rewind();
			return false;
		}
		prev_source = e.u;
		while (!right_endpoints.empty() && *right_endpoints < e.u) {
			++right_endpoints;
		}
		if (!right_endpoints.empty() && *right_endpoints == e.u) {
			if (!e.self_loop()) {
				edges.rewind();
				return false;
			}
		}
	}
	edges.rewind();
	return true;
}

template <typename edge_stream_t, class edge_comparator_t>
bool is_sorted(edge_stream_t& edges, edge_comparator_t cmp) {
	edge_t prev = cmp.min_value();
	for (; !edges.empty(); ++edges) {
		if (!cmp(prev, *edges)) {
			std::cout << prev << " not cmp " << *edges << std::endl;
			edges.rewind();
			return false;
		}
		prev = *edges;
	}
	edges.rewind();
	return true;
}
