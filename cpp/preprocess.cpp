#include <iostream>
#include <numeric>
#include <random>

#include <stxxl/algorithm>
#include <stxxl/sort>

#include "contraction.hpp"
#include "defs.hpp"
#include "util.hpp"


int main(int argc, char *argv[]) {
	if (argc < 3) {
		std::cout << "Not enough args; please specify input and output graph files" << std::endl;
		return 0;
	}

	std::mt19937_64 gen(std::random_device{}());

	em_edge_vector E;
	read_graph(argv[1], E);
	stxxl::sort(E.begin(), E.end(), edge_lt_ordering(), INTERNAL_SORT_MEM);
	em_node_vector map_from = unique_nodes(E);
	node_t n = map_from.size();
	std::cout << "Graph has " << n << " nodes and " << E.size() << " edges" << std::endl;

	em_node_vector map_to(n);
	std::iota(map_to.begin(), map_to.end(), 1);
	auto n_rand = to_stxxl_rand(gen);
	stxxl::random_shuffle(map_to.begin(), map_to.end(), n_rand, INTERNAL_SORT_MEM);
	auto u_it = map_from.begin();
	auto v_it = map_to.begin();
	em_edge_vector map;
	while (u_it != map_from.end()) {
		map.push_back(edge_t{*u_it, *v_it});
		++u_it;
		++v_it;
	}

	relabel(E, map);
	orient_smaller_to_larger(E);
	stxxl::sort(E.begin(), E.end(), edge_lt_ordering(), INTERNAL_SORT_MEM);

	write_graph(E, argv[2]);
}
