#include <iostream>

#include <foxxll/io.hpp>
#include <foxxll/io/iostats.hpp>
#include <tlx/cmdline_parser.hpp>
#include <stxxl/stream>

#include "defs.hpp"
#include "util.hpp"


int main(int argc, char *argv[]) {
	tlx::CmdlineParser cp;
	cp.set_description("Measure how long it takes to iterate through edges");

	std::string input_filename;
	cp.add_param_string("input", input_filename, "Input graph file");

	if (!cp.process(argc, argv)) {
		return -1;
	}

	foxxll::scoped_print_iostats global_stats("total");
	foxxll::file_ptr input_file = tlx::make_counting<foxxll::syscall_file>(input_filename, foxxll::file::RDONLY | foxxll::file::DIRECT);
	const em_edge_vector E(input_file);
	std::cout << "Have mapped vector of " << E.size() << " edges" << std::endl;

	node_t sum = 0;
	{
		foxxll::scoped_print_iostats global_stats("iterate over streamify");
		using input_edge_stream_t = stxxl::stream::vector_iterator2stream<em_edge_vector::const_iterator>;
		input_edge_stream_t input_stream = stxxl::stream::streamify(E.begin(), E.end());
		for (; !input_stream.empty(); ++input_stream) {
			sum += (*input_stream).u;
		}
	}
	std::cout << "Sum: " << sum << std::endl;

	sum = 0;
	{
		foxxll::scoped_print_iostats global_stats("iterate with const auto&");
		for (const auto& e: E) {
			sum += e.u;
		}
	}
	std::cout << "Sum: " << sum << std::endl;

	return 0;
}
