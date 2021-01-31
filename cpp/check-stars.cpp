#include <iostream>

#include <fstream>

#include "defs.hpp"
#include "robin_hood.h"


int main(int argc, char *argv[]) {
	if (argc < 2) {
		std::cout << "Not enough args; please specify input stars file and optionally whether to output size counts rather than root: star size for each star" << std::endl;
		return 0;
	}


	robin_hood::unordered_map<node_t,size_t> sizes;
	robin_hood::unordered_map<node_t,bool> selfloop;
	{
		std::ifstream in(argv[1]);
		node_t edge[2];
		while (in.read((char*)edge, bytes_per_edge)) {
			node_t& v = edge[1];
			auto lookup = sizes.find(v);
			if (lookup == sizes.end()) {
				sizes[v] = 1;
				selfloop[v] = false;
			} else {
				sizes[v]++;
			}
			if (edge[0] == v) {
				selfloop[v] = true;
			}
		}
	}
	if (argc > 2) {
		robin_hood::unordered_map<size_t,size_t> sizecounts;
		for (auto pair: sizes) {
			size_t size = pair.second;
			if (!selfloop[pair.first]) {
				size++;
			}
			auto lookup = sizecounts.find(size);
			if (lookup == sizecounts.end()) {
				sizecounts[size] = 1;
			} else {
				sizecounts[size]++;
			}
		}
		for (auto pair: sizecounts) {
			std::cout << pair.first << " " << pair.second << std::endl;
		}
	} else {
		for (auto pair: sizes) {
			if (selfloop[pair.first]) {
				std::cout << pair.first << ": " << pair.second << " ✓" << std::endl;
			} else {
				std::cout << pair.first << ": " << pair.second << " ✗" << std::endl;
			}
		}
	}

}
