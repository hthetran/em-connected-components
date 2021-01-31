#include <iostream>

#include <foxxll/io.hpp>
#include <foxxll/io/iostats.hpp>
#include <tlx/cmdline_parser.hpp>

#include "defs.hpp"
#include "util.hpp"
#include "variants.hpp"
#include "streaming/containers/EdgeStream.h"
#include "streaming/contraction/StarContraction.h"
#include "streaming/FunctionalSubproblemManager.h"

int main(int argc, char *argv[]) {
	tlx::CmdlineParser cp;
	cp.set_description("Run a (streaming) variant of the \"functional\" CC algorithm on a graph");

	std::string input_filename;
	cp.add_param_string("input", input_filename, "Input graph file");

	size_t semiexternal_size;
	cp.add_param_size_t("basecase", semiexternal_size, "Number of nodes for which to switch to semi-external algorithm");

	size_t num_nodes = 0;
	cp.add_size_t("num_nodes", num_nodes, "Number of nodes in input graph");

	std::string output_filename = "";
	cp.add_opt_param_string("output", output_filename, "Output graph file");

	unsigned algorithm_variant = 0;
	cp.add_unsigned("variant", algorithm_variant, "Version of algorithm to use (0/1/2)");

	unsigned seed = std::random_device{}();
	cp.add_unsigned("seed", seed, "Random seed to use");

	if (!cp.process(argc, argv)) {
		return -1;
	}

	if (num_nodes == 0) {
		std::cout << "Node counting temporarily disabled, please specify using num_nodes" << std::endl;
		return -1;
	}

	std::cout << "Running with seed " << seed << std::endl;
	foxxll::scoped_print_iostats global_stats("total");
	EdgeStream input_stream;
	size_t num_edges;
	{
		foxxll::scoped_print_iostats read_stats("read_graph");
		num_edges = read_graph_to_stream(input_filename, input_stream);
		input_stream.consume();
	}

	std::cout << "Graph has " << num_nodes << " nodes and " << num_edges << " edges" << std::endl;
	bool save_output = (output_filename != "");
	if (!save_output) {
		std::cout << "Output will not be saved" << std::endl;
	}

	em_mapping cc_map; // to hold result
	node_t num_counted_nodes = 0; // for debugging
	{
		foxxll::scoped_print_iostats alg_stats("algorithm");
		policy_t policy = variant_policies[algorithm_variant];
		std::mt19937_64 gen(seed);

		// note: parameter given is number of bytes of main memory
		// TODO: calculate probability in each recursion level
		double p = policy.sample_prob(num_nodes, num_edges, 0);
		FunctionalSubproblemManager<EdgeStream, StarContraction> funman(p, input_stream, sizeof(node_t) * semiexternal_size, num_nodes, policy, seed);
		for (; !funman.empty(); ++funman) {
			const auto node_label = *funman;
			++num_counted_nodes;
			cc_map.push_back(edge_t(node_label.node, node_label.load));
		}
	}
	std::cout << "num_nodes " << num_nodes << std::endl;
	std::cout << "num_counted_nodes " << num_counted_nodes << std::endl;

	if (save_output) {
		write_graph(cc_map, output_filename);
	}
	return 0;
}
