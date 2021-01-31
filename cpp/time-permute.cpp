#include <iostream>
#include <random>

#include <foxxll/io.hpp>
#include <foxxll/io/iostats.hpp>
#include <tlx/cmdline_parser.hpp>
#include <stxxl/algorithm>

#include "defs.hpp"
#include "util.hpp"


int main(int argc, char *argv[]) {
	tlx::CmdlineParser cp;
	cp.set_description("Permute an edge list file");

	std::string input_filename;
	cp.add_param_string("input", input_filename, "Input graph file");

	if (!cp.process(argc, argv)) {
		return -1;
	}

	foxxll::scoped_print_iostats global_stats("total");
	foxxll::file_ptr input_file = tlx::make_counting<foxxll::syscall_file>(input_filename, foxxll::file::RDWR | foxxll::file::DIRECT);
	em_edge_vector E(input_file);
	std::mt19937_64 gen(std::random_device{}());
	auto n_rand = to_stxxl_rand(gen);

	stxxl::random_shuffle(E.begin(), E.end(), n_rand, INTERNAL_SORT_MEM);
}
