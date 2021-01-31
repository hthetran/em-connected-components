#include <iostream>
#include <fstream>
#include <tlx/cmdline_parser.hpp>

#include "defs.hpp"

int main(int argc, char* argv[]) {
	tlx::CmdlineParser cp;
	cp.set_description("Generate a graph consisting of disjoint cliques");

	size_t clique_size;
	cp.add_param_size_t("clique_size", clique_size, "Number of nodes per clique");

	size_t num_cliques;
	cp.add_param_size_t("num_cliques", num_cliques, "Number of disjoint cliques");

	std::string output_filename;
	cp.add_param_string("output", output_filename, "Output graph file");

	if (!cp.process(argc, argv)) {
		return -1;
	}

	std::ofstream out(output_filename, std::ios::binary);

	node_t edge[2];
	for (node_t clique=0; clique<num_cliques; ++clique) {
		node_t first = (clique*clique_size)+1;
		node_t last = first+clique_size-1;
		for (node_t u=first; u<last; ++u) {
			edge[0] = u;
			for (node_t v=u+1; v<=last; ++v) {
				edge[1] = v;
				out.write(reinterpret_cast<char*>(&edge), sizeof(edge));
			}
		}
	}
}
