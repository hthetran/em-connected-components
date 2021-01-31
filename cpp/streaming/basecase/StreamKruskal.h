/*
 * StreamKruskal.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#pragma once

#include "BaseKruskal.h"

class StreamKruskal final : public BaseKruskal {
public:
    template <typename ComponentsSorter, typename... InEdges>
    void process(ComponentsSorter& out_comps, InEdges&& ... in_streams) {
        tlx::call_foreach(
        [&](auto&& in_stream) { process_edge_stream(std::forward<decltype(in_stream)>(in_stream)); },
        std::forward<InEdges>(in_streams) ...
        );
        process_output(out_comps);
    }

private:
    template <typename EdgeStream>
    void process_edge_stream(EdgeStream& edges) {
        for (; !edges.empty(); ++edges) {
            const auto edge = *edges;
            const node_t u = use_map(edge.u);
            const node_t v = use_map(edge.v);
            if (op_union(u, v)) {
                ++_num_unions;
            }
        }
    }
};