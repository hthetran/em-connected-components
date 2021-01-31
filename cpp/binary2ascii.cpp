#include <fstream>
#include <iostream>

#include "defs.hpp"


int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cout << "Not enough args; please specify input (binary) file" << std::endl;
		return 0;
	}

	std::ifstream in(argv[1], std::ios::binary);
	node_t edge[2];
	while (in.read((char*)edge, bytes_per_edge)) {
		std::cout << edge[0] << " " << edge[1] << std::endl;
	}
}
