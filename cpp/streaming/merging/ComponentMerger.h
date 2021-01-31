/*
 * ComponentMerger.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#pragma once

#include <type_traits>
#include "../hungdefs.hpp"
#include "../utils/StreamFilter.h"
#include "../utils/StreamPusher.h"
#include "../transforms/make_unique_stream.h"

class ComponentMerger {
public:
    template <
        typename LeftComponentSorter,
        typename RightComponentStream,
        typename OutMergedComponents
    >
    ComponentMerger(LeftComponentSorter& left, RightComponentStream& right, OutMergedComponents& out) {
        static_assert(std::is_same<typename LeftComponentSorter::value_type, node_component_t>::value,
                      "Sorter requires value_type that contains (node, load).");
        static_assert(std::is_same<typename LeftComponentSorter::cmp_type, node_component_cc_node_less_cmp>::value,
                      "Sorter requires cmp_type that sorts by (cc, node).");
        static_assert(std::is_same<typename RightComponentStream::value_type, node_component_t>::value,
                      "Right components require value_type that contains (node, load).");

        using out_value_type = typename OutMergedComponents::value_type;

        make_unique_stream<LeftComponentSorter> left_uqe(left);
        make_unique_stream<RightComponentStream> right_uqe(right);

        // left: (u, v), right_uqe: (v, w), out: (u, w)
        for (; !right_uqe.empty(); ++right_uqe) {
            const auto right_uqe_label = *right_uqe;
            out.push(right_uqe_label);
            for (; !left_uqe.empty(); ++left_uqe) {
                const auto left_uqe_label = *left_uqe;
                if (left_uqe_label.load < right_uqe_label.node)
                    out.push(left_uqe_label);
                else if (left_uqe_label.load == right_uqe_label.node)
                    out.push(out_value_type{left_uqe_label.node, right_uqe_label.load});
                else
                    break;
            }
        }

        // flush left
        StreamPusher(left_uqe, out);

        // asserts
        assert(left_uqe.empty());
        assert(right_uqe.empty());
    }

};