#include <iostream>

#include <foxxll/common/timer.hpp>
#include <foxxll/io/iostats.hpp>
#include <tlx/cmdline_parser.hpp>

#include "defs.hpp"
#include "util.hpp"
#include "streaming/containers/EdgeStream.h"
#include "stream-functional.hpp"
#include "variants.hpp"


int main(int argc, char *argv[]) {
	tlx::CmdlineParser cp;
	cp.set_description("Run a variant of the streaming implementaiton of the \"functional\" CC algorithm on a graph");

	std::string input_filename;
	cp.add_param_string("input", input_filename, "Input graph file");

	size_t semiexternal_size;
	cp.add_param_size_t("basecase", semiexternal_size, "Number of nodes for which to switch to semi-external algorithm");

	std::string output_filename = "";
	cp.add_opt_param_string("output", output_filename, "Output graph file");

	unsigned algorithm_variant = 0;
	cp.add_unsigned("variant", algorithm_variant, "Version of algorithm to use (0-3)");

	if (!cp.process(argc, argv)) {
		return -1;
	}

	em_edge_vector E = read_graph(input_filename);
	node_t num_nodes = number_of_nodes(E);
	std::cout << "Graph has " << num_nodes << " nodes and " << E.size() << " edges" << std::endl;
	bool save_output = (output_filename != "");
	if (!save_output) {
		std::cout << "Output will not be saved" << std::endl;
	}

	policy_t policy = variant_policies[algorithm_variant];

	EdgeStream input_stream(false, true);
	for (auto& e: E) {
		input_stream.push(e);
	}
	input_stream.consume();
	std::cout << "Edges now in stream" << std::endl;

	foxxll::timer timer;
	foxxll::scoped_print_iostats global_stats("total");
	std::cout << "operation,input,output,time" << std::endl;
	timer.start();
	EdgeStream result_stream;
	compute_ccs(input_stream, num_nodes, semiexternal_size, result_stream, 0, policy);
	result_stream.consume();

	// TODO: something more clever; and timing it in a fair way
	em_mapping cc_map;
    for (; !result_stream.empty(); ++result_stream) {
	    cc_map.push_back(*result_stream);
    }
	timer.stop();
	std::cout << "total,,," << timer.useconds() << std::endl;
	timer.reset();

	if (save_output) {
		write_graph(cc_map, output_filename);
	}
	return 0;
}
