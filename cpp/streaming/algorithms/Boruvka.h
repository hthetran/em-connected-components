/*
 * Boruvka.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#pragma once

#include <memory>

#include "../../defs.hpp"
#include "../hungdefs.hpp"
#include "../merging/ComponentMerger.h"
#include "../contraction/BoruvkaContraction.h"
#include "../basecase/StreamKruskal.h"

class Boruvka {
public:
    using edge_sorter_less_t = stxxl::sorter<edge_t, edge_less_cmp>;
    using node_cc_sorter_node_cc_less_t = stxxl::sorter<node_component_t, node_component_node_cc_less_cmp>;
    using node_cc_sorter_cc_node_less_t = stxxl::sorter<node_component_t, node_component_cc_node_less_cmp>;
    using value_type = node_component_t;

    template <typename EdgesIn>
    Boruvka(EdgesIn & edges_, node_t num_nodes_, size_t main_memory_size_)
    : num_nodes(num_nodes_),
      num_edges(edges_.size()),
      main_memory_size(main_memory_size_),
      component_map(node_component_node_cc_less_cmp(), SORTER_MEM),
      output(nullptr)
    {
        if (is_semi_externally_handleable(num_nodes)) {
            StreamKruskal semiext_kruskal;
            semiext_kruskal.process(component_map, edges_);
            component_map.sort_reuse();
            output.reset(new make_unique_stream(component_map));
        } else {
            BoruvkaContraction boruvka_contraction_algo;
            edge_sorter_less_t contracted_edges(edge_less_cmp(), SORTER_MEM);
            node_cc_sorter_node_cc_less_t boruvka_contraction(node_component_node_cc_less_cmp(), SORTER_MEM);
            boruvka_contraction_algo.compute_fully_external_contraction(edges_, contracted_edges, boruvka_contraction, 0);
            contracted_edges.sort_reuse();

            node_cc_sorter_cc_node_less_t boruvka_contraction_ccless(node_component_cc_node_less_cmp(), SORTER_MEM);
            StreamPusher(boruvka_contraction, boruvka_contraction_ccless);
            boruvka_contraction_ccless.sort_reuse();

            const node_t num_ccs = boruvka_contraction_algo.get_node_upper_bound();
            Boruvka subproblem(contracted_edges, num_ccs, main_memory_size);

            ComponentMerger(boruvka_contraction_ccless, subproblem, component_map);
            component_map.sort_reuse();

            output.reset(new make_unique_stream(component_map));
        }
    }

    [[nodiscard]] bool empty() const {
        return output->empty();
    }

    const value_type& operator * () const {
        return output->operator*();
    }

    Boruvka& operator ++ (){
        output->operator++();
        return *this;
    }

    void rewind() {
        output->rewind();
        output.reset(new make_unique_stream(component_map));
    }

private:
    node_t num_nodes;
    const size_t num_edges;
    const size_t main_memory_size;
    node_cc_sorter_node_cc_less_t component_map;
    std::unique_ptr<make_unique_stream<node_cc_sorter_node_cc_less_t>> output;

    bool is_semi_externally_handleable(node_t num_nodes_subproblem) const {
	    return main_memory_size / (sizeof(node_t) * num_nodes_subproblem * StreamKruskal::MEMORY_OVERHEAD_FACTOR);
    }
};
