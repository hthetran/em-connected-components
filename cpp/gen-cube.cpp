#include <iostream>
#include <fstream>
#include <tlx/cmdline_parser.hpp>

#include "defs.hpp"

int main(int argc, char* argv[]) {
	tlx::CmdlineParser cp;
	cp.set_description("Generate a \"cube\" graph (a number of disjoint layers of generalized grid graphs)");

	size_t width;
	cp.add_param_size_t("width", width, "Width of layers");

	size_t height;
	cp.add_param_size_t("height", height, "Height of layers");

	size_t layers;
	cp.add_param_size_t("layers", layers, "Number of layers");

	size_t distance = 1;
	cp.add_size_t("distance", distance, "How many neighbors over to connect to (incl. diagonal)");

	std::string output_filename;
	cp.add_param_string("output", output_filename, "Output graph file");

	if (!cp.process(argc, argv)) {
		return -1;
	}

	std::ofstream out(output_filename, std::ios::binary);

	node_t edge[2];
	for (node_t layer=0; layer<layers; ++layer) {
		node_t layer_start = layer*width*height+1;
		for (node_t row=0; row<height; ++row) {
			for (node_t col=0; col<width; ++col) {
				node_t source = row*width + col + layer_start;
				edge[0] = source;
				// go right
				for (size_t col_offset=1; col_offset <= distance && (col_offset+col) < width; ++col_offset) {
					edge[1] = source + col_offset;
					out.write((char*)(&edge[0]), bytes_per_edge);
				}
				// go to all below
				size_t start_col = col >= distance ? col-distance : 0;
				size_t end_col = col+distance < width ? col+distance : width-1;
				for (size_t row_offset=1; row_offset <= distance && (row_offset+row) < height; ++row_offset) {
					for (size_t target_col=start_col; target_col <= end_col; ++target_col) {
						edge[1] = row*width + row_offset*width + target_col + layer_start;
						out.write((char*)(&edge[0]), bytes_per_edge);
					}
				}
			}
		}
	}
}
