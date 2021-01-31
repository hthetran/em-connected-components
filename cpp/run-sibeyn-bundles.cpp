/*
 * run-sibeyn-bundles.cpp
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#include <iostream>

#include <foxxll/io/iostats.hpp>
#include <tlx/cmdline_parser.hpp>

#include "streaming/containers/Bundles.h"
#include "streaming/SibeynWithBundles.h"
#include "kruskal.hpp"
#include "defs.hpp"
#include "util.hpp"


int main(int argc, char *argv[]) {
    tlx::CmdlineParser cp;
    cp.set_description("Run the modified \"Sibeyn/Meyer\" CC with bundles algorithm on a graph");

    std::string input_filename;
    cp.add_param_string("input", input_filename, "Input graph file");

	size_t internal_memory_bytes;
	cp.add_param_bytes("memory", internal_memory_bytes, "Internal memory budget (bytes)");

    node_t max_id = MIN_NODE;
    cp.add_size_t("max_id", max_id, "Maximum node ID in input graph");

    std::string output_filename = "";
    cp.add_opt_param_string("output", output_filename, "Output graph file");

    unsigned buffer_variant = 0;
    cp.add_unsigned("variant", buffer_variant, "Buffer type to use: 0 (default) means most buffers, 3 least buffers, [1, 2] interpolation between most and least");

    bool minimize_interbundle_edges = false;
    cp.add_flag("minimize", minimize_interbundle_edges, "Minimize interbundle edges");

    if (!cp.process(argc, argv)) {
        return -1;
    }

    // min: semi-external algorithm takes up *all* of M
    size_t min_num_bundles = (max_id * BoundedIntervalKruskal::MEMORY_OVERHEAD_FACTOR * sizeof(node_t)) / internal_memory_bytes;
    // max: bundle buffers take up half of M
    size_t max_num_bundles = (internal_memory_bytes / 2) / (2 * EquiRangedBundles::BUNDLE_BLOCK_SIZE);
    if (min_num_bundles == 0) {
	    std::cout << "Note: min #bundles is <1, could just run Kruskal directly" << std::endl;
	    std::cout << "Upping min to 1 bundles" << std::endl;
	    min_num_bundles = 1;
    }
    if (min_num_bundles > max_num_bundles) {
	    std::cout << "Not enough memory!" << std::endl;
	    std::cout << "Minimum number of bundles (to be able to run Kruskal on each): " << min_num_bundles << std::endl;
	    std::cout << "Maximum number of bundles (to fit buffers into M/2): " << max_num_bundles << std::endl;
	    std::cout << "Memory given (in bytes): " << internal_memory_bytes << std::endl;
	    std::cout << "Block size (in bytes):   " << EquiRangedBundles::BUNDLE_BLOCK_SIZE << std::endl;
	    return -1;
    }
    std::cout << "Number of bundles should be between " << min_num_bundles << " and " << max_num_bundles << std::endl;
    size_t num_bundles = 0;
    switch (buffer_variant) {
    case 0:
	    num_bundles = max_num_bundles;
	    break;
    case 1:
	    num_bundles = (min_num_bundles + 2*max_num_bundles)/3;
	    break;
    case 2:
	    num_bundles = (2*min_num_bundles + max_num_bundles)/3;
	    break;
    case 3:
	    num_bundles = min_num_bundles;
	    break;
    default:
	    std::cout << "Illegal variant " << buffer_variant << std::endl;
	    return -1;
    }
    std::cout << "Number of bundles chosen: " << num_bundles << std::endl;

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
        if (max_id == MIN_NODE) {
            std::cout << "Maximum node ID not specified, will scan first..." << std::endl;
            for (const auto& e: E) {
	            if (e.v > max_id) {
		            max_id = e.v;
	            }
            }
        }
        num_edges = E.size();
    }

    std::cout << "Minimization is" << (minimize_interbundle_edges ? "" : " not") << " enabled" << std::endl;
    std::cout << "Graph has max node ID " << max_id << " nodes and " << num_edges << " edges" << std::endl;
    bool save_output = (output_filename != "");
    if (!save_output) {
        std::cout << "Output will not be saved" << std::endl;
    }

    em_mapping cc_map; // to hold result
    node_t num_counted_nodes = 0; // for debugging
    {
        foxxll::scoped_print_iostats alg_stats("algorithm");
        SibeynWithBundles<EquiRangedBundles> sibeyn_with_bundles(edge_stream, max_id, internal_memory_bytes, num_bundles, minimize_interbundle_edges);
        for (; !sibeyn_with_bundles.empty(); ++sibeyn_with_bundles) {
            const auto node_label = *sibeyn_with_bundles;
            ++num_counted_nodes;
            cc_map.push_back(edge_t{node_label.u, node_label.v});
        }
    }
    std::cout << "max_node_id " << max_id << std::endl;
    std::cout << "num_counted_nodes " << num_counted_nodes << std::endl;

    if (save_output) {
        write_graph(cc_map, output_filename);
    }
    return 0;
}
