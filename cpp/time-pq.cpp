#include <iostream>

#include <foxxll/mng/read_write_pool.hpp>
#include <foxxll/io.hpp>
#include <foxxll/io/iostats.hpp>
#include <tlx/cmdline_parser.hpp>
#include <stxxl/priority_queue>

#include "defs.hpp"
#include "util.hpp"


int main(int argc, char *argv[]) {
	tlx::CmdlineParser cp;
	cp.set_description("Measure sorting a file using a PQ");

	std::string input_filename;
	cp.add_param_string("input", input_filename, "Input graph file");

	std::string output_filename = "";
	cp.add_opt_param_string("output", output_filename, "Output graph file");

	if (!cp.process(argc, argv)) {
		return -1;
	}

	foxxll::scoped_print_iostats global_stats("total");
	foxxll::file_ptr input_file = tlx::make_counting<foxxll::syscall_file>(input_filename, foxxll::file::RDONLY | foxxll::file::DIRECT);
	const em_edge_vector E(input_file);
	std::cout << "Have mapped vector of " << E.size() << " edges" << std::endl;
	std::cout << "INTERNAL_PQ_MEMORY is " << INTERNAL_PQ_MEM << std::endl;

	bool save_output = (output_filename != "");
	if (!save_output) {
		std::cout << "Output will not be saved" << std::endl;
	}

	using pq_type = stxxl::PRIORITY_QUEUE_GENERATOR<edge_t, edge_gt_lt_ordering, INTERNAL_PQ_MEM, MAX_PQ_SIZE>::result;
	using block_type = pq_type::block_type;
    const auto PQ_POOL_MEM_HALF_BLOCKS = (PQ_POOL_MEM / 2) / block_type::raw_size;
    foxxll::read_write_pool<block_type> pool(PQ_POOL_MEM_HALF_BLOCKS, PQ_POOL_MEM_HALF_BLOCKS);
	pq_type pq(pool);
	{
		foxxll::scoped_print_iostats fill_pq("fill PQ");
		for (const auto& e: E) {
			pq.push(e);
		}
	}

	em_edge_vector sorted;
	{
		edge_t e;
		foxxll::scoped_print_iostats fill_pq("empty PQ");
		while (!pq.empty()) {
			e = pq.top();
			sorted.push_back(e);
			pq.pop();
		}
	}

	if (save_output) {
		write_graph(sorted, output_filename);
	}

	return 0;
}
