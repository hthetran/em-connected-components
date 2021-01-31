#include <iostream>

#include <fstream>

#include <stxxl/sorter>
#include "defs.hpp"
#include "util.hpp"
#include "streaming/containers/EdgeStream.h"

int main(int argc, char *argv[]) {
	if (argc < 3) {
        std::cout << "Not enough args; please specify two input star files" << std::endl;
		return 0;
	}

    stxxl::sorter<edge_t, edge_less_cmp> edges_l(edge_less_cmp(), SORTER_MEM);
    stxxl::sorter<edge_t, edge_less_cmp> edges_r(edge_less_cmp(), SORTER_MEM);
    {
        std::ifstream in(argv[1]);
        node_t edge[2];
        while (in.read((char*)edge, bytes_per_edge)) {
            node_t& u = edge[0];
            node_t& v = edge[1];
            edges_l.push(edge_t{u, v});
        }
    }
    {
        std::ifstream in(argv[2]);
        node_t edge[2];
        while (in.read((char*)edge, bytes_per_edge)) {
            node_t& u = edge[0];
            node_t& v = edge[1];
            edges_r.push(edge_t{u, v});
        }
    }

    std::cout << "edges_l.size() " << edges_l.size() << std::endl;
    std::cout << "edges_r.size() " << edges_r.size() << std::endl;
    edges_l.sort_reuse();
    edges_r.sort_reuse();
    node_t last_l = MAX_NODE;
    node_t last_r = MAX_NODE;
    node_t max_l = 0;
    node_t max_r = 0;
    for (; !edges_l.empty(); ++edges_l, ++edges_r) {
        const auto edge_l = *edges_l;
        const auto edge_r = *edges_r;

        max_l = std::max(edge_l.u, max_l);
        max_r = std::max(edge_r.u, max_r);

        if (edge_l.u == last_l) {
            std::cout << "same node in left" << std::endl;
        }

        if (edge_r.u == last_r) {
            std::cout << "same node in right" << std::endl;
        }
       
        if (edge_l.u != edge_r.u) { 
            if (edge_l.u != last_l + 1 && edge_l.u != MAX_NODE) {
                std::cout << "LEFT not consecutive nodes, last " << last_l << " next " << edge_l.u << std::endl;
            }

            if (edge_r.u != last_r + 1 && edge_r.u != MAX_NODE) {
                std::cout << "RIGHT not consecutive nodes, last " << last_r << " next " << edge_r.u << std::endl;
            }
        }

        if (edge_l.u != edge_r.u) {
            std::cout << "difference: " << edge_l << " " << edge_r << std::endl;
            if (edge_r.u < edge_l.u) {
                std::cout << "forwarded right stream by 1" << std::endl;
                ++edges_r;
                std::cout << "Now " << (*edges_r).u << " " << (*edges_r).v << std::endl;
            }
            /* else {
                std::cout << "forwarded left stream by 1" << std::endl;
                ++edges_l;
                std::cout << "Now " << (*edges_l).u << " " << (*edges_l).v << std::endl;
            }*/
        }

        last_l = edge_l.u;
        last_r = edge_r.u;
    }
    std::cout << "left edges done" << std::endl;
    std::cout << "printing, remaining right edges" << std::endl;
    for (; !edges_r.empty(); ++edges_r) {
        std::cout << *edges_r << std::endl;
    }

    std::cout << "max_l " << max_l << std::endl;
    std::cout << "max_r " << max_r << std::endl;


    return 0;
}
