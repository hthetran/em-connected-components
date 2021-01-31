#include <iostream>

#include <foxxll/io.hpp>
#include <stxxl/sort>
#include <tlx/cmdline_parser.hpp>

#include "defs.hpp"
#include "util.hpp"


int main(int argc, char *argv[]) {
	tlx::CmdlineParser cp;
	cp.set_description("Count the number of nodes and edges of a graph");

	std::string input_filename;
	cp.add_param_string("input", input_filename, "Input graph file");

	bool fully_external;
	cp.add_flag("external", fully_external, "Whether to use fully-external sort-based counting");

	if (!cp.process(argc, argv)) {
		return -1;
	}

	foxxll::file_ptr input_file = tlx::make_counting<foxxll::syscall_file>(input_filename, foxxll::file::RDONLY | foxxll::file::DIRECT);
	const em_edge_vector E(input_file);
	size_t num_nodes = 0;
	node_t max_node_seen = MIN_NODE;
	if (fully_external) {
		std::tie(num_nodes, max_node_seen) = external_number_of_nodes(E);
	} else {
		std::tie(num_nodes, max_node_seen) = internal_number_of_nodes(E);
	}
	std::cout << "number of nodes," << num_nodes << std::endl;
	std::cout << "number of edges," << E.size() << std::endl;
	std::cout << "max node id," << max_node_seen << std::endl;

	return 0;
}
