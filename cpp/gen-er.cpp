#include <iostream>
#include <fstream>
#include <random>

#include <tlx/cmdline_parser.hpp>

#include "defs.hpp"


int main(int argc, char* argv[]) {
	tlx::CmdlineParser cp;
	cp.set_description("Generate a Gilbert graph from parameters n and density");

	size_t n;
	cp.add_param_size_t("n", n, "Number of nodes");

	double ratio;
	cp.add_param_double("ratio", ratio, "Target density");

	std::string output_filename;
	cp.add_param_string("output", output_filename, "Output graph file");

	unsigned seed = std::random_device{}();
	cp.add_unsigned("seed", seed, "Random seed for generator");

	if (!cp.process(argc, argv)) {
		return -1;
	}

	double p = 2*ratio/(static_cast<double>(n-1));
	std::ofstream out(output_filename, std::ios::binary);

	std::random_device rand_dev;
	std::default_random_engine gen{ rand_dev() };
	std::geometric_distribution<size_t> dist(p);
	size_t row_width = n-1;
	node_t edge[2];
	// index 0 is "u", index 1 is "v"
	edge[0] = 1;
	node_t v_offset = 0;
	while (true) {
		size_t skip = dist(gen);
		v_offset += skip;
		while (v_offset >= row_width) {
			++edge[0];
			v_offset -= row_width;
			--row_width;
		}
		if (edge[0] > n) {
			break;
		}
		edge[1] = edge[0]+v_offset+1;
		out.write((char*)(&edge[0]), bytes_per_edge);
		v_offset++;
		if (v_offset >= row_width) {
			edge[0]++;
			v_offset -= row_width;
			row_width--;
		}
	}
}
