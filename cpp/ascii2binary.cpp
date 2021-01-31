#include <fstream>
#include <iostream>
#include <string>

#include <tlx/cmdline_parser.hpp>

#include "defs.hpp"

int main(int argc, char* argv[]) {
	tlx::CmdlineParser cp;
	cp.set_description("Read ASCII edge list file and output binary file");

	std::string input_filename;
	cp.add_param_string("input", input_filename, "Input ASCII file");

	std::string output_filename;
	cp.add_param_string("output", output_filename, "Output binary file name");

	size_t skip_lines = 0;
	cp.add_size_t("skip", skip_lines, "Optionally skip lines at beginning of file");

	node_t subtract_index = 0;
	cp.add_size_t("subtract", subtract_index, "Optionally decrease indices by this amount");

	node_t add_index = 0;
	cp.add_size_t("add", add_index, "Optionally increase indices by this amount (only relevant if minimum index was < 1 such as 0)");

	if (!cp.process(argc, argv)) {
		return -1;
	}

	std::ifstream in(input_filename);
	std::ofstream out(output_filename, std::ios::binary);
	std::string line;
	node_t edge[2];

	// skip some lines
	for (size_t i=0; i<skip_lines; ++i) {
		std::getline(in, line);
	}

	while (std::getline(in, line)) {
		char* sep = line.data();
		edge[0] = std::strtoull(sep, &sep, 10) + add_index - subtract_index;
		edge[1] = std::strtoull(sep, NULL, 10) + add_index - subtract_index;
		assert(edge[0] > MIN_NODE && edge[0] < MAX_NODE);
		assert(edge[1] > MIN_NODE && edge[1] < MAX_NODE);
		out.write((char*)(&edge[0]), bytes_per_edge);
	}
}
