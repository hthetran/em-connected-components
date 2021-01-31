#include <iostream>
#include <random>

#include <stxxl/sort>
#include <tlx/cmdline_parser.hpp>

#include "contraction.hpp"
#include "kruskal.hpp"
#include "util.hpp"


int main(int argc, char *argv[]) {
	tlx::CmdlineParser cp;
	cp.set_description("Run the \"Boruvka\"-inspired CC algorithm on a graph");

	std::string input_filename;
	cp.add_param_string("input", input_filename, "Input graph file");

	size_t semiexternal_size;
	cp.add_param_size_t("basecase", semiexternal_size, "Number of nodes for which to switch to semi-external algorithm");

	std::string output_filename = "";
	cp.add_opt_param_string("output", output_filename, "Output graph file");

	unsigned seed = std::random_device{}();
	cp.add_unsigned("seed", seed, "Random seed to use");

	if (!cp.process(argc, argv)) {
		return -1;
	}

	em_edge_vector E = read_graph(input_filename);
	node_t num_nodes = number_of_nodes(E);
	std::vector<em_edge_vector*> stars; // different levels; 0 is first set of stars

	std::cout << "Graph has " << num_nodes << " nodes and " << E.size() << " edges" << std::endl;
	bool save_output = (output_filename != "");
	if (!save_output) {
		std::cout << "Output will not be saved" << std::endl;
	}
	std::cout << "operation,input,output,time" << std::endl;
	foxxll::timer timer;
	foxxll::timer global_timer;
	global_timer.start();

	std::mt19937_64 gen(seed);

	while (num_nodes > semiexternal_size && E.size() > semiexternal_size) {
		std::cout << "iteration," << num_nodes << "," << E.size() << "," << std::endl;
		timer.start();
		em_edge_vector* sample = sample_out_edges(E, gen);
		timer.stop();
		std::cout << "sample_out," << E.size() << "," << sample->size() << "," << timer.useconds() << std::endl;
		timer.reset();

		size_t size_pre = sample->size();
		timer.start();
		break_paths(*sample);
		timer.stop();
		std::cout << "break_paths," << size_pre << "," << sample->size() << "," << timer.useconds() << std::endl;
		timer.reset();

		stars.push_back(sample);

		size_pre = E.size();
		timer.start();
		contract_stars(E, *sample);
		timer.stop();
		num_nodes = num_nodes - sample->size();
		std::cout << "contract," << size_pre << "," << E.size() << "," << timer.useconds() << std::endl;
		timer.reset();
	}

	{
		timer.start();
		em_edge_vector* cc_map = new em_edge_vector;
		stars.push_back(cc_map);
		SEMKruskal kruskal(E, 0, E.size(), *cc_map);
		stxxl::sort(cc_map->begin(), cc_map->end(), edge_lt_ordering(), INTERNAL_SORT_MEMORY);
		timer.stop();
		std::cout << "kruskal," << E.size() << "," << cc_map->size() << "," << timer.useconds() << std::endl;
		timer.reset();
	}

	for (unsigned long i=stars.size()-1; i>0; --i) {
		timer.start();
		relabel_targets(*stars[i-1], *stars[i]);
		for (const edge_t& e: *stars[i]) {
			stars[i-1]->push_back(e);
		}
		stxxl::sort(stars[i-1]->begin(), stars[i-1]->end(), edge_lt_ordering(), INTERNAL_SORT_MEMORY);
		timer.stop();
		std::cout << "update_map," << stars[i]->size() << "," << stars[i-1]->size() << "," << timer.useconds() << std::endl;
		timer.reset();
	}

	global_timer.stop();
	std::cout << "total,,," << global_timer.useconds() << std::endl;

	if (save_output) {
		write_graph(*stars[0], output_filename);
	}
}
