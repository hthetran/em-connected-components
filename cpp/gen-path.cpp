#include <iostream>
#include <fstream>

#include <foxxll/io.hpp>
#include <foxxll/io/iostats.hpp>
#include <tlx/cmdline_parser.hpp>

#include "defs.hpp"

int main(int argc, char* argv[]) {
	tlx::CmdlineParser cp;
	cp.set_description("Generate a path graph");

	node_t n;
	cp.add_param_size_t("n", n, "Number of nodes");

	std::string output_filename;
	cp.add_param_string("output", output_filename, "Output graph file");

	if (!cp.process(argc, argv)) {
		return -1;
	}

	foxxll::scoped_print_iostats global_stats("total");
	foxxll::file::unlink(output_filename.c_str());
	foxxll::file_ptr output_file = tlx::make_counting<foxxll::syscall_file>(output_filename, foxxll::file::WRONLY | foxxll::file::CREAT | foxxll::file::DIRECT);
	em_edge_vector E(output_file);
	E.reserve(n-1);

	edge_t e(1, 2);
	for (node_t u=1; u<n; ++u) {
		E.push_back(e);
		++e.u;
		++e.v;
	}

}
