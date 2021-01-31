#include <iostream>
#include <fstream>
#include <tlx/cmdline_parser.hpp>
#include <foxxll/io.hpp>
#include <stxxl/comparator>
#include <stxxl/sorter>

#include "defs.hpp"
#include "robin_hood.h"

int main(int argc, char *argv[]) {
	tlx::CmdlineParser cp;
	cp.set_description("Output histogram of degrees for nodes present in edge list");

	std::string input_filename;
	cp.add_param_string("input", input_filename, "Input graph file");

	std::string output_filename = "";
	cp.add_opt_param_string("output", output_filename, "Output csv file (stdout by default)");

	if (!cp.process(argc, argv)) {
		return -1;
	}

	using comp = stxxl::comparator<node_t>;
	stxxl::sorter<node_t, comp> sorter(comp(), 2*UIntScale::Gi);

	foxxll::file_ptr input_file = tlx::make_counting<foxxll::syscall_file>(input_filename, foxxll::file::RDONLY | foxxll::file::DIRECT);
	const em_edge_vector E(input_file);

	std::cout << "Read ... " << std::flush;
	for (const auto& e: E) {
		assert(e.u < e.v);
		assert(e.u > MIN_NODE);
		assert(e.v < MAX_NODE);
		sorter.push(e.u);
		sorter.push(e.v);
	}
	std::cout << "done." << std::endl;

	if (sorter.size() < 2) {
		std::cout << "No edges found\n";
		return -1;
	}

	std::cout << "Sort ... " << std::flush;
	sorter.sort();
	std::cout << "done." << std::endl;

	node_t prev = *sorter;
	size_t degree = 1;
	++sorter;

	std::map<node_t, size_t> degree_distr;
	for (; !sorter.empty(); ++sorter) {
		auto node = *sorter;
		if (node != prev) {
			degree_distr[degree]++;
			degree = 0;
			prev = node;
		}
		++degree;
	}
	degree_distr[degree]++;

	if (output_filename != "") {
		std::ofstream fd(output_filename);
		for (auto [degree, count] : degree_distr) {
			fd << degree << "," << count << std::endl;
		}
	} else {
		for (auto [degree, count] : degree_distr) {
			std::cout << degree << "," << count << std::endl;
		}
	}

	return 0;
}
