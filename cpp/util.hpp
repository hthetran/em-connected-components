#pragma once

#include <foxxll/io.hpp>
#include <fstream>

#include <stxxl/sort>
#include "defs.hpp"
#include "robin_hood.h"

template <typename stream_type>
size_t read_graph_to_stream(std::string input_filename, stream_type& input_stream) {
	foxxll::file_ptr input_file = tlx::make_counting<foxxll::syscall_file>(input_filename, foxxll::file::RDONLY | foxxll::file::DIRECT);
	const em_edge_vector E(input_file);

	size_t num_parallel = 0;
	size_t num_edges = 0;
	edge_t prev = edge_t{MIN_NODE, MIN_NODE};
	for (const auto& e: E) {
		if (TLX_LIKELY(e != prev)) {
			input_stream.push(e);
			prev = e;
			++num_edges;
		} else {
			++num_parallel;
		}
	}
	if (num_parallel > 0) {
		std::cout << "Dropped " << num_parallel << " parallel edges" << std::endl;
	}
	return num_edges;
}

void read_graph(std::string fn, em_edge_vector& E) {
	foxxll::file_ptr input_file = tlx::make_counting<foxxll::syscall_file>(fn, foxxll::file::RDONLY | foxxll::file::DIRECT);
	const em_edge_vector mapped(input_file);
	E.resize(mapped.size());
	em_edge_vector::bufwriter_type bw(E);
	size_t num_parallel = 0;
	edge_t prev = edge_t{MIN_NODE, MIN_NODE};
	for (const auto& e: mapped) {
		if (TLX_LIKELY(e != prev)) {
			bw << e;
			prev = e;
		} else {
			++num_parallel;
		}
		if (num_parallel > 0) {
			std::cout << "Dropped " << num_parallel << " parallel edges" << std::endl;
		}
	}
	bw.finish();
}

void write_graph(const em_edge_vector& E, std::string fn) {
	std::ofstream out(fn, std::ios::binary);
	node_t edge[2];
	// TODO: something smarter
	for (auto e: E) {
		edge[0] = e.u;
		edge[1] = e.v;
		out.write((char*)(&edge[0]), bytes_per_edge);
	}
}

void orient_smaller_to_larger(em_edge_vector& E) {
	for (size_t i=0; i<E.size(); i++) {
		if (E[i].u > E[i].v) {
			std::swap(E[i].u, E[i].v);
		}
	}
}

void orient_larger_to_smaller(em_edge_vector& E) {
	for (size_t i=0; i<E.size(); i++) {
		if (E[i].u < E[i].v) {
			std::swap(E[i].u, E[i].v);
		}
	}
}

std::ostream& operator<< (std::ostream& out, const edge_t& e) {
	out << e.u << "," << e.v;
	return out;
}

std::pair<size_t, node_t> external_number_of_nodes(const em_edge_vector& E) {
	size_t num_unique = 0;
	node_t max_node_seen = MIN_NODE;
	if (E.size() == 0) {
		return std::make_pair(num_unique, max_node_seen);
	}
	// actually extract all node IDs and count unique...
	em_node_vector V;
	node_t prev_source = MIN_NODE;
	for (auto& e: E) {
		if (e.u != prev_source) {
			V.push_back(e.u);
			prev_source = e.u;
		}
		V.push_back(e.v);
		if (e.v > max_node_seen) {
			max_node_seen = e.v;
		}
	}
	stxxl::sort(V.begin(), V.end(), node_lt_ordering(), INTERNAL_SORT_MEM);
	node_t prev_node = MIN_NODE;
	for (node_t& u: V) {
		if (u != prev_node) {
			num_unique++;
			prev_node = u;
		}
	}
	return std::make_pair(num_unique, max_node_seen);
}

std::pair<size_t, node_t> internal_number_of_nodes(const em_edge_vector& E) {
	robin_hood::unordered_set<node_t> node_set;
	node_t max_node_seen = MIN_NODE;
	for (const auto& e: E) {
		node_set.insert(e.u);
		node_set.insert(e.v);
		if (e.v > max_node_seen) {
			max_node_seen = e.v;
		}
	}
	size_t num_unique = node_set.size();
	return std::make_pair(num_unique, max_node_seen);
}

em_node_vector unique_nodes(const em_edge_vector& E) {
	// actually extract all node IDs and count unique...
	em_node_vector V;
	node_t prev_source = MIN_NODE;
	for (auto& e: E) {
		if (e.u != prev_source) {
			V.push_back(e.u);
			prev_source = e.u;
		}
		V.push_back(e.v);
	}
	stxxl::sort(V.begin(), V.end(), node_lt_ordering(), INTERNAL_SORT_MEM);
	node_t prev_node = MIN_NODE;
	size_t next_index = 0;
	for (node_t& u: V) {
		if (u != prev_node) {
			V[next_index] = u;
			prev_node = u;
			++next_index;
		}
	}
	V.resize(next_index);
	return V;
}

auto to_stxxl_rand(std::mt19937_64 &gen) {
	return [&gen] (auto x) {return std::uniform_int_distribution<decltype(x)>{0, x-1}(gen);};
}
