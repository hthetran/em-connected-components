#include <iostream>
#include <fstream>

#include "defs.hpp"

int main(int argc, char* argv[]) {
	if (argc < 4) {
		std::cout << "Not enough args; please specify n, m, and output file" << std::endl;
		return 0;
	}
	size_t n = std::stol(argv[1]);
	size_t m = std::stol(argv[2]);
	std::ofstream out(argv[3], std::ios::binary);
	node_t edge[2];

	for (node_t row=0; row<m-1; ++row) {
		for (node_t col=0; col<n-1; ++col) {
			edge[0] = row*n + col + 1;
			edge[1] = edge[0]+1;
			out.write((char*)(&edge[0]), bytes_per_edge);
			edge[1] = edge[0]+n;
			out.write((char*)(&edge[0]), bytes_per_edge);
		}
		// down from last in row to last in next row
		edge[0] = (row+1)*n;
		edge[1] = (row+2)*n;
		out.write((char*)(&edge[0]), bytes_per_edge);
	}
	for (node_t col=0; col<n-1; ++col) {
		edge[0] = (m-1)*n + col + 1;
		edge[1] = edge[0]+1;
		out.write((char*)(&edge[0]), bytes_per_edge);
	}
}
