#include <iostream>

#include <stxxl/stream>
#include <foxxll/io.hpp>
#include <foxxll/io/iostats.hpp>
#include <tlx/cmdline_parser.hpp>

#include "streaming/basecase/StreamKruskal.h"
#include "streaming/containers/EdgeSequence.h"
#include "streaming/containers/EdgeStream.h"
#include "streaming/contraction/Sibeyn.hpp"
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

	if (num_nodes == 0) {
		std::cout << "Node counting temporarily disabled, please specify using num_nodes" << std::endl;
		return -1;
	}

	foxxll::scoped_print_iostats global_stats("total");
	EdgeStream input_stream;
	{
		foxxll::scoped_print_iostats read_stats("read_graph");
		read_graph_to_stream(input_filename, input_stream);
		input_stream.consume();
	}
	EdgeStream tree; // output of Sibeyn
	EdgeSequence leftover; // left in PQ after Sibeyn
	// note: leftover left as sequence as it may not be sorted (violates invariants on EdgeStream)
	EdgeSequence intermediate_stars; // stars reported by Kruskal to be fed to tfp
	using EdgeSorter = stxxl::sorter<edge_t, edge_lt_ordering>;
	EdgeSorter stars(edge_lt_ordering(), SORTER_MEM); // for final output after tfp

	std::cout << "Graph has " << num_nodes << " nodes and " << input_stream.size() << " edges" << std::endl;
	size_t semiext_nodes = internal_memory_bytes / (sizeof(node_t) * StreamKruskal::MEMORY_OVERHEAD_FACTOR);
	std::cout << "Will contract down to " << semiext_nodes << " nodes before base case" << std::endl;
	bool save_output = (output_filename != "");
	if (!save_output) {
		std::cout << "Output will not be saved" << std::endl;
	}

	{
		foxxll::scoped_print_iostats alg_stats("algorithm");
		{
			foxxll::scoped_print_iostats stats("sibeyn_contractions");
			run_sibeyn_tuned(input_stream, num_nodes - semiext_nodes, tree, leftover);
			tree.consume();
		}

		{
			foxxll::scoped_print_iostats stats("kruskal");
			StreamKruskal kruskal_algo;
			leftover.rewind();
			kruskal_algo.process(intermediate_stars, leftover);
		}

		{
			foxxll::scoped_print_iostats stats("tfp");
			tree.rewind();
			StreamEdgesOrientReverse reversed_tree_edges(tree);
			intermediate_stars.rewind();
			tfp_after_basecase(reversed_tree_edges, intermediate_stars, stars);
			stars.sort();
		}
	}

	em_mapping cc_map(stars.size());
	stxxl::stream::materialize(stars, cc_map.begin(), cc_map.end());
	if (save_output) {
		write_graph(cc_map, output_filename);
	}

	return 0;
}
