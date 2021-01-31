#include <iostream>

#include <foxxll/io.hpp>
#include <foxxll/io/iostats.hpp>
#include <tlx/cmdline_parser.hpp>
#include <stxxl/sort>

#include "defs.hpp"
#include "util.hpp"


int main(int argc, char *argv[]) {
	tlx::CmdlineParser cp;
	cp.set_description("Measure sorting a file using stxxl::sort");

	std::string input_filename;
	cp.add_param_string("input", input_filename, "Input graph file");

	if (!cp.process(argc, argv)) {
		return -1;
	}

	foxxll::scoped_print_iostats global_stats("total");
	foxxll::file_ptr input_file = tlx::make_counting<foxxll::syscall_file>(input_filename, foxxll::file::RDONLY | foxxll::file::DIRECT);
	const em_edge_vector E(input_file);
	std::cout << "Have mapped vector of " << E.size() << " edges" << std::endl;
	std::cout << "INTERNAL_SORT_MEM is " << INTERNAL_SORT_MEM << std::endl;

	em_edge_vector sorted;
	{
		foxxll::scoped_print_iostats global_stats("copy to new vector using bufwriter");
		sorted.resize(E.size());
		em_edge_vector::bufwriter_type writer(sorted);
		for (const auto& e: E) {
			writer << e;
		}
	}
	std::cout << "Size of new vector:" << std::endl;
	std::cout << sorted.size() << std::endl;

	{
		foxxll::scoped_print_iostats global_stats("sort new vector");
		std::cout << "Length of the vector to sort is " << sorted.size() << std::endl;
		stxxl::sort(sorted.begin(), sorted.end(), edge_lt_ordering(), INTERNAL_SORT_MEM);
	}

	return 0;
}
