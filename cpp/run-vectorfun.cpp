#include <iostream>
#include <unordered_map>

#include <foxxll/common/timer.hpp>
#include <foxxll/io/iostats.hpp>
#include <tlx/cmdline_parser.hpp>

#include "defs.hpp"
#include "util.hpp"
#include "functional.hpp"
#include "variants.hpp"


int main(int argc, char *argv[]) {
	tlx::CmdlineParser cp;
	cp.set_description("Run a variant of the \"functional\" CC algorithm on a graph");

	std::string input_filename;
	cp.add_param_string("input", input_filename, "Input graph file");

	size_t semiexternal_size;
	cp.add_param_size_t("basecase", semiexternal_size, "Number of nodes for which to switch to semi-external algorithm");

	size_t num_nodes = 0;
	cp.add_size_t("num_nodes", num_nodes, "Number of nodes in input graph");

	std::string output_filename = "";
	cp.add_opt_param_string("output", output_filename, "Output graph file");

	unsigned algorithm_variant = 0;
	cp.add_unsigned("variant", algorithm_variant, "Version of algorithm to use (0-3)");

	unsigned seed = std::random_device{}();
	cp.add_unsigned("seed", seed, "Random seed to use");

	if (!cp.process(argc, argv)) {
		return -1;
	}

	std::cout << "Running with seed " << seed << std::endl;
	foxxll::scoped_print_iostats global_stats("total");
	em_edge_vector E;
	{
		foxxll::scoped_print_iostats read_stats("read_graph");
		read_graph(input_filename, E);
		if (num_nodes == 0) {
			std::cout << "Will explicitly count number of nodes" << std::endl;
			num_nodes = number_of_nodes(E);
		}
	}
	std::cout << "Graph has " << num_nodes << " nodes and " << E.size() << " edges" << std::endl;
	bool save_output = (output_filename != "");
	if (!save_output) {
		std::cout << "Output will not be saved" << std::endl;
	}
	std::cout << "operation,input,output,time" << std::endl;

	em_mapping cc_map; // to hold result
	{
		foxxll::scoped_print_iostats alg_stats("algorithm");
		foxxll::timer timer;
		timer.start();
		policy_t policy = variant_policies[algorithm_variant];
		std::mt19937_64 gen(seed);

		FunctionalConnectedComponents algo(E, num_nodes, semiexternal_size, gen, 0, policy);
		algo.run(cc_map);
		timer.stop();
		std::cout << "total,,," << timer.useconds() << std::endl;
		timer.reset();
	}

	if (save_output) {
		write_graph(cc_map, output_filename);
	}
	return 0;
}
