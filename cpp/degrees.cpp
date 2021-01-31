#include <fstream>
#include <iostream>
#include <map>

#include "defs.hpp"

int main(int argc, char *argv[]) {
	if (argc != 2) {
		std::cout << "Please give graph parameter" << std::endl;
		return -1;
	}

	std::map<node_t, size_t> counts;
	auto increment_count = [&](node_t x) {
		auto lookup = counts.find(x);
		if (lookup == counts.end()) {
			counts[x] = 1;
		} else {
			counts[x]++;
		}
	};
	std::ifstream in(argv[1], std::ios::binary);
	node_t edge[2];
	while (in.read(reinterpret_cast<char *>(&edge), 2*sizeof(node_t))) {
		assert(edge[0] < edge[1]);
		assert(edge[0] > 0);
		increment_count(edge[0]);
		increment_count(edge[1]);
	}
	for (const auto& pair: counts) {
		std::cout << pair.first << "," << pair.second << std::endl;
	}
	return 0;
}
