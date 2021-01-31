#include <tlx/cmdline_parser.hpp>

#include <foxxll/io.hpp>
#include <foxxll/io/iostats.hpp>

#include "defs.hpp"
#include "util.hpp"

int main(int argc, char *argv[]) {
	tlx::CmdlineParser cp;
	cp.set_description("Remove self-loops from file");

	std::string input_filename;
	cp.add_param_string("input", input_filename, "Input graph file");

	std::string output_filename;
	cp.add_param_string("output", output_filename, "Output graph file");

	if (!cp.process(argc, argv)) {
		return -1;
	}

	foxxll::file_ptr input_file = tlx::make_counting<foxxll::syscall_file>(input_filename, foxxll::file::RDONLY);
	const em_edge_vector E_in(input_file);

	foxxll::file_ptr output_file = tlx::make_counting<foxxll::syscall_file>(output_filename, foxxll::file::RDWR | foxxll::file::CREAT);
	em_edge_vector E_out(output_file);

	size_t num_loops = 0;
	size_t num_parallel = 0;
	em_edge_vector::bufwriter_type writer(E_out);
	edge_t prev = edge_t(MIN_NODE, MIN_NODE);
	{
		foxxll::scoped_print_iostats global_stats("processing");
		for (const auto& e: E_in) {
			if (TLX_UNLIKELY(e.self_loop())) {
				++num_loops;
			} else if (TLX_UNLIKELY(prev == e)) {
				++num_parallel;
			} else if (TLX_UNLIKELY(!(prev <= e))) {
				std::cout << "Uh oh, " << e << " < " << prev  << std::endl;
			} else {
				writer << e;
			}
			prev = e;
		}
	}
	std::cout << "Found " << num_loops << " self-loops" << std::endl;
	std::cout << "Found " << num_parallel << " parallel edges" << std::endl;
}
