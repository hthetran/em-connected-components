#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

#include <foxxll/io.hpp>
#include <stxxl/sort>
#include <tlx/cmdline_parser.hpp>

#include "defs.hpp"
#include "util.hpp"
#include "robin_hood.h"


int main(int argc, char *argv[]) {
	tlx::CmdlineParser cp;
	cp.set_description("Permute node IDs of a graph (overwriting)");

	std::string filename;
	cp.add_param_string("file", filename, "Graph file to randomize");

	unsigned seed = std::random_device{}();
	cp.add_unsigned("seed", seed, "Random seed for shuffling IDs");

	size_t num_nodes = 0;
	cp.add_size_t("num_nodes", num_nodes, "Number of nodes (avoids recounting)");

	if (!cp.process(argc, argv)) {
		return -1;
	}

	std::mt19937_64 gen(seed);
	// opening graph
	foxxll::file_ptr input_file = tlx::make_counting<foxxll::syscall_file>(filename, foxxll::file::RDWR | foxxll::file::DIRECT);
	em_edge_vector E(input_file);

	node_t max_id = 0;
	if (num_nodes == 0) {
		std::cout << "Did not provide number of nodes, will explicitly count..." << std::endl;
		std::tie(num_nodes, max_id) = internal_number_of_nodes(E);
		std::cout << "Graph has " << num_nodes << " unique nodes" << std::endl;
		std::cout << "max id is " << max_id << std::endl;
	}

	// generating random node IDs
	std::vector<node_t> map_to(num_nodes);
	std::iota(map_to.begin(), map_to.end(), 1);
	std::shuffle(map_to.begin(), map_to.end(), gen);

	// run through while building map /and/ relabeling
	robin_hood::unordered_map<node_t, node_t> mapping;
	auto next_id = map_to.begin();
	auto get_new_id = [&](node_t x) {
		auto lookup = mapping.find(x);
		node_t new_id;
		if (lookup == mapping.end()) {
			if (next_id == map_to.end()) {
				std::cout << "num_nodes supplied seems too low!" << std::endl;
			}
			new_id = *next_id;
			++next_id;
			mapping[x] = new_id;
		} else {
			new_id = lookup->second;
		}
		return new_id;
	};
	for (auto& e: E) {
		node_t new_u = get_new_id(e.u);
		node_t new_v = get_new_id(e.v);
		e.u = new_u;
		e.v = new_v;
		e.orient_smaller_to_larger();
	}

	// now sort the thing again
	stxxl::sort(E.begin(), E.end(), edge_lt_ordering(), INTERNAL_SORT_MEM);
}
