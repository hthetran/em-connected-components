#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

#include <foxxll/io.hpp>
#include <tlx/cmdline_parser.hpp>
#include <foxxll/mng/read_write_pool.hpp>

#include <stxxl/priority_queue>

#include "defs.hpp"
#include "util.hpp"


void run_sibeyn_forward(const em_edge_vector& input_edges, em_edge_vector& out_tree, unsigned variant, std::mt19937_64& gen);

int main(int argc, char *argv[]) {
	tlx::CmdlineParser cp;
	cp.set_description("Run versions of \"Sibeyn\" CC algorithm and track interesting stats");

	unsigned variant = 0;
	cp.add_param_unsigned("variant", variant, "0 for first neighbor, 1 for random, 2 for last");

	std::string input_filename;
	cp.add_param_string("input", input_filename, "Input graph file");

	unsigned seed = std::random_device{}();
	cp.add_unsigned("seed", seed, "Random seed to use");

	std::string output_filename = "";
	cp.add_opt_param_string("output", output_filename, "Output graph file");

	if (!cp.process(argc, argv)) {
		return -1;
	}

	foxxll::file_ptr input_file = tlx::make_counting<foxxll::syscall_file>(input_filename, foxxll::file::RDONLY | foxxll::file::DIRECT);
	const em_edge_vector E(input_file);
	em_edge_vector sibeyn_tree; // output of Sibeyn

	bool save_output = (output_filename != "");
	if (!save_output) {
		std::cout << "Output will not be saved" << std::endl;
	}

	std::cout << "Have seed " << seed << " (only applicable to random neighbor choosing)" << std::endl;
	std::mt19937_64 gen(seed);
	run_sibeyn_forward(E, sibeyn_tree, variant, gen);

	if (save_output) {
		// TODO: do we care about output?
	}

	return 0;
}

void run_sibeyn_forward(const em_edge_vector& input_edges, em_edge_vector& out_tree, unsigned variant, std::mt19937_64& gen) {
	using pq_type = stxxl::PRIORITY_QUEUE_GENERATOR<edge_t, edge_gt_ordering, INTERNAL_PQ_MEM, MAX_PQ_SIZE>::result;
	using block_type = pq_type::block_type;
	const auto pool_half_mem = PQ_POOL_MEM / 2 / block_type::raw_size;
	foxxll::read_write_pool<block_type> pool(pool_half_mem, pool_half_mem);
	pq_type pq(pool);
	for (const auto& e: input_edges) {
		assert(e.u < e.v);
		pq.push(e);
	}

	std::cout << "node,neighbors,duplicates" << std::endl;
	size_t pq_operations = 0;
	while (!pq.empty()) {
		// outer loop: contract all sources
		// each iteration should be looking at new source
		edge_t e = pq.top();
		pq.pop();
		++pq_operations;
		edge_t prev = e;
		node_t current_source = e.u;
		std::vector<node_t> neighbors;
		neighbors.push_back(e.v);
		size_t duplicate_edges = 0;
		while (!pq.empty()) {
			// inner loop: gather all remaining neighbors of original_u
			e = pq.top();
			if (e.u != current_source) {
				break;
			}
			++pq_operations;
			pq.pop();
			if (e == prev) {
				++duplicate_edges;
				continue;
			}
			prev = e;
			neighbors.push_back(e.v);
		}
		std::cout << current_source << "," << neighbors.size() << "," << duplicate_edges << std::endl;

		// now choose a neighbor to be target for contraction
		node_t contract_to_v = 0;
		if (variant == 0) {
			contract_to_v = neighbors.front();
		} else if (variant == 1) {
			std::uniform_int_distribution<> dist(0, neighbors.size()-1);
			size_t index = dist(gen);
			contract_to_v = neighbors[index];
		} else if (variant == 2) {
			contract_to_v = neighbors.back();
		} else {
			std::cout << "Uh oh" << std::endl;
		}
		edge_t tree_edge(current_source, contract_to_v);
		assert(tree_edge.u < tree_edge.v);
		out_tree.push_back(tree_edge);
		for (const node_t v: neighbors) {
			if (v == contract_to_v) {
				// this would just be self-loop
				continue;
			}
			// forward this edge
			edge_t new_edge(v, contract_to_v);
			new_edge.orient_smaller_to_larger();
			assert(current_source < new_edge.u);
			pq.push(new_edge);
		}
	}
	std::cout << "done" << std::endl;
	std::cout << "pq_ops," << pq_operations << std::endl;
}
