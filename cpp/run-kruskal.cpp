#include <iostream>

#include <foxxll/io.hpp>
#include <foxxll/common/timer.hpp>

#include "defs.hpp"
#include "util.hpp"
#include "kruskal.hpp"


using component_map_t = std::unordered_map<node_t, node_t>;

int main(int argc, char *argv[]) {
	if (argc < 3) {
		std::cout << "Not enough args; please specify input, output graph files" << std::endl;
		return 0;
	}

	foxxll::file_ptr input_file = tlx::make_counting<foxxll::syscall_file>(argv[1], foxxll::file::RDONLY | foxxll::file::DIRECT);
	const em_edge_vector E(input_file);
	foxxll::file::unlink(argv[2]);
	foxxll::file_ptr output_file = tlx::make_counting<foxxll::syscall_file>(argv[2], foxxll::file::RDWR | foxxll::file::CREAT | foxxll::file::DIRECT);
	em_edge_vector forest(output_file);
	std::cout << "Graph has " << E.size() << " edges" << std::endl;

	foxxll::timer timer;
	timer.start();
	em_mapping cc_map;
	SEMKruskal algo(E, 0, E.size(), cc_map);
	timer.stop();
	std::cout << "kruskal,"
	          << E.size() << ","
	          << cc_map.size() << ","
	          << timer.useconds() << std::endl;
	timer.reset();

	// TODO: improve this
	timer.start();
	for (const auto& e: cc_map) {
		if (e.u != e.v) {
			forest.push_back(edge_t(e.u, e.v));
		}
	}
	timer.stop();
	std::cout << "convert_edges,,," << timer.useconds() << std::endl;

	return 0;
}
