#include <iostream>

#include <foxxll/io.hpp>
#include <foxxll/io/iostats.hpp>
#include <tlx/cmdline_parser.hpp>
#include <stxxl/stream>
#include <stxxl/sorter>

#include "defs.hpp"
#include "util.hpp"


template <typename edge_stream_t, class edge_comparator_t>
bool is_sorted(edge_stream_t& edges, edge_comparator_t cmp) {
	edge_t prev = cmp.min_value();
	for (; !edges.empty(); ++edges) {
		if (!cmp(prev, *edges)) {
			std::cout << prev << " not cmp " << *edges << std::endl;
			return false;
		}
		prev = *edges;
	}
	return true;
}

int main(int argc, char *argv[]) {
	tlx::CmdlineParser cp;
	cp.set_description("Measure sorting a file using streams");

	std::string input_filename;
	cp.add_param_string("input", input_filename, "Input graph file");

	if (!cp.process(argc, argv)) {
		return -1;
	}

	foxxll::scoped_print_iostats global_stats("total");
	foxxll::file_ptr input_file = tlx::make_counting<foxxll::syscall_file>(input_filename, foxxll::file::RDONLY | foxxll::file::DIRECT);
	const em_edge_vector E(input_file);
	em_edge_vector sorted;
	sorted.resize(E.size());
	std::cout << "Have mapped vector of " << E.size() << " edges" << std::endl;
	std::cout << "INTERNAL_SORT_MEM is " << INTERNAL_SORT_MEM << std::endl;

	{
		foxxll::scoped_print_iostats global_stats("stream::sort");
		std::cout << "Streamifying vector iterators" << std::endl;
		using input_edge_stream_t = stxxl::stream::vector_iterator2stream<em_edge_vector::const_iterator>;
		input_edge_stream_t input_stream = stxxl::stream::streamify(E.begin(), E.end());
		std::cout << "Making a sort stream" << std::endl;
		using edge_sorter_t = stxxl::stream::sort<input_edge_stream_t, edge_lt_ordering>;
		edge_sorter_t sorted_edges(input_stream, edge_lt_ordering(), SORTER_MEM);
		std::cout << "Running through to ensure it's merged" << std::endl;
		for (; !sorted_edges.empty(); ++sorted_edges) {}
	}

	{
		foxxll::scoped_print_iostats global_stats("sorter");
		std::cout << "Making a sorter" << std::endl;
		using edge_sorter_t = stxxl::sorter<edge_t, edge_lt_ordering>;
		edge_sorter_t sorted_edges(edge_lt_ordering(), SORTER_MEM);
		std::cout << "Push edges into sorter" << std::endl;
		for (const auto& e: E) {
			sorted_edges.push(e);
		}
		std::cout << "Sorting" << std::endl;
		sorted_edges.sort();
		std::cout << "Running through to ensure it's merged" << std::endl;
		for (; !sorted_edges.empty(); ++sorted_edges) {}
	}

	return 0;
}
