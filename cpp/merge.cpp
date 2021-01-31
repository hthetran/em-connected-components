#include <fstream>
#include <iostream>

#include "defs.hpp"
#include <stxxl/sorter>

int main(int argc, char *argv[]) {
  if (argc < 4) {
    std::cout << "merge outfile <in0> <in1>" << std::endl;
    return -1;
  }

  using edge_target_sorter = stxxl::sorter<edge_t, edge_target_lt_ordering>;
  edge_target_sorter sorter(edge_target_lt_ordering(), 1llu << 30);

  size_t edges_read = 0;
  for (int i = 2; i < argc; ++i) {
    std::cout << "Read file " << argv[i] << " ... " << std::flush;
    std::ifstream in(argv[i], std::ios::binary);
    node_t edge[2];

    size_t read = 0;
    while (in.read((char *)edge, bytes_per_edge)) {
      if (edge[0] > edge[1])
        std::swap(edge[0], edge[1]);

      sorter.push(edge_t{edge[0], edge[1]});
      read++;
    }

    std::cout << read << " edges read. done." << std::endl;
    edges_read += read;
  }

  std::cout << "Edge read: " << edges_read << std::endl;

  std::cout << "Sort ... " << std::flush;
  sorter.sort();
  std::cout << "done." << std::endl;

  std::cout << "Write result ... " << std::endl;
  std::ofstream out(argv[1], std::ios::binary);
  edge_t prev_edge = MAX_EDGE;

  for (; !sorter.empty(); ++sorter) {
    auto edge = *sorter;
    if (prev_edge == edge)
      continue;

    out.write(reinterpret_cast<char *>(&edge), sizeof(edge));

    prev_edge = edge;
  }

  std::cout << "done." << std::endl;

  return 0;
}
