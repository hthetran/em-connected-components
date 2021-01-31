#include <iostream>

#include <foxxll/io.hpp>
#include <foxxll/io/iostats.hpp>
#include <tlx/cmdline_parser.hpp>

#include "defs.hpp"
#include "util.hpp"
#include "streaming/containers/EdgeStream.h"
#include "streaming/algorithms/Boruvka.h"

int main(int argc, char *argv[]) {
	tlx::CmdlineParser cp;
	cp.set_description("Run a (streaming) variant of the \"functional\" CC algorithm on a graph");

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
	EdgeStream edge_stream;
	size_t num_edges;
	{
		foxxll::scoped_print_iostats read_stats("read_graph");
		foxxll::file_ptr input_file = tlx::make_counting<foxxll::syscall_file>(input_filename, foxxll::file::RDONLY | foxxll::file::DIRECT);
		const em_edge_vector E(input_file);
		for (const auto& e: E) {
			edge_stream.push(e);
		}
		edge_stream.rewind();
		if (num_nodes == 0) {
			std::cout << "Will explicitly count number of nodes" << std::endl;
			num_nodes = external_number_of_nodes(E).first;
		}
		num_edges = E.size();
	}

	std::cout << "Graph has " << num_nodes << " nodes and " << num_edges << " edges" << std::endl;
	bool save_output = (output_filename != "");
	if (!save_output) {
		std::cout << "Output will not be saved" << std::endl;
	}

	node_t num_counted_nodes = 0;
	em_mapping cc_map; // to hold result
	{
		foxxll::scoped_print_iostats alg_stats("algorithm");

		// note: parameter given is number of bytes of main memory
		Boruvka boruvka_algo(edge_stream, num_nodes, internal_memory_bytes);
		for (; !boruvka_algo.empty(); ++boruvka_algo) {
			const auto node_label = *boruvka_algo;
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
