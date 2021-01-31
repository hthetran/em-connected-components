#include <iostream>

#include <foxxll/io.hpp>
#include <stxxl/sort>
#include <tlx/cmdline_parser.hpp>

#include "defs.hpp"


int main(int argc, char *argv[]) {
	tlx::CmdlineParser cp;
	cp.set_description("Report maximum node ID for a graph file");

	std::string input_filename;
	cp.add_param_string("input", input_filename, "Input graph file");

	if (!cp.process(argc, argv)) {
		return -1;
	}

	foxxll::file_ptr input_file = tlx::make_counting<foxxll::syscall_file>(input_filename, foxxll::file::RDONLY | foxxll::file::DIRECT);
	const em_edge_vector E(input_file);
	node_t max_node_seen = MIN_NODE;
	for (const auto& e: E) {
		if (e.v > max_node_seen) {
			max_node_seen = e.v;
		}
	}
	std::cout << "max node id," << max_node_seen << std::endl;

	return 0;
}
