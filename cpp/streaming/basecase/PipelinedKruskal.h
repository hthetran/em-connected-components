/*
 * PipelinedKruskal.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#pragma once

#include "BaseKruskal.h"

class PipelinedKruskal final : public BaseKruskal {
public:
    void push(edge_t edge) {
        const node_t u = use_map(edge.u);
        const node_t v = use_map(edge.v);
        if (op_union(u, v)) {
            ++_num_unions;
        }
    }

    template <typename ComponentsSorter>
    void process(ComponentsSorter& out_comps) {
        process_output(out_comps);
    }

    node_t get_first_inserted_node() const {
        return (!_reverse_map.empty() ? _reverse_map[0] : MAX_NODE);
    }

	template <typename OutputMap>
	void process_to_map(OutputMap& output) {
		for (node_t i = 0; i < _reverse_map.size(); ++i) {
			const node_t u = _reverse_map[i];
			const node_t root = _reverse_map[op_find(i)];
			output[u] = root;
		}
	}
};
