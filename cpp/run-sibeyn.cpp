#include <iostream>

#include <foxxll/io/iostats.hpp>
#include <tlx/cmdline_parser.hpp>

#include "sibeyn.hpp"
#include "kruskal.hpp"
#include "defs.hpp"
#include "util.hpp"

int main(int argc, char *argv[]) {
	tlx::CmdlineParser cp;
	cp.set_description("Run the modified \"Sibeyn/Meyer\" CC algorithm on a graph");

	std::string input_filename;
	cp.add_param_string("input", input_filename, "Input graph file");

	size_t internal_memory_bytes;
	cp.add_param_bytes("memory", internal_memory_bytes, "Internal memory budget (bytes)");

	size_t num_nodes = 0;
	cp.add_size_t("num_nodes", num_nodes, "Number of nodes in input graph");

	std::string output_filename = "";
	cp.add_opt_param_string("output", output_filename, "Output graph file");

	if (!cp.process(argc, argv)) {
		return -1;
	}

	foxxll::scoped_print_iostats global_stats("total");
	em_edge_vector E;
	{
		foxxll::scoped_print_iostats read_stats("read_graph");
		read_graph(input_filename, E);
		if (num_nodes == 0) {
			std::cout << "Will explicitly count number of nodes" << std::endl;
			num_nodes = external_number_of_nodes(E).first;
		}
	}
	em_edge_vector tree; // output of Sibeyn
	em_edge_vector leftover; // left in PQ after Sibeyn
	em_edge_vector stars; // for final output after tfp

	std::cout << "Graph has " << num_nodes << " nodes and " << E.size() << " edges" << std::endl;
	size_t semiext_nodes = internal_memory_bytes / (sizeof(node_t) * SEMKruskal::MEMORY_OVERHEAD_FACTOR);
	std::cout << "Will contract down to " << semiext_nodes << " nodes before base case" << std::endl;
	bool save_output = (output_filename != "");
	if (!save_output) {
		std::cout << "Output will not be saved" << std::endl;
	}

	{
		foxxll::scoped_print_iostats alg_stats("algorithm");
		{
			foxxll::scoped_print_iostats stats("sibeyn_contractions");
			run_sibeyn(num_nodes - semiext_nodes, E, tree, leftover);
		}

		{
			foxxll::scoped_print_iostats stats("kruskal");
			SEMKruskal kruskal(leftover, 0, leftover.size(), stars); // Kruskal gives us some stars
		}

		{
			foxxll::scoped_print_iostats stats("tfp");
			orient_larger_to_smaller(tree);
			tfp(tree, stars);
		}
	}

	if (save_output) {
		write_graph(stars, output_filename);
	}

	return 0;
}
