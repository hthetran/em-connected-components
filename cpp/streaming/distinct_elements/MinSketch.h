/*
 * MinSketch.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#ifndef EM_CC_MINSKETCH_H
#define EM_CC_MINSKETCH_H

#include "../utils/Int32ArithmeticHashFamily.h"

template <typename HashFunction, typename NodeType>
class NewMinSketch {
public:
    template <typename Gen>
    NewMinSketch(Gen& gen, NodeType max_node)
     : h(gen, max_node),
       h_max(max_node)
    { }

    void operator() (NodeType x)  {
        h_min = std::min(h_min, static_cast<NodeType>(h(x)));
    }

    NodeType count() const {
        return static_cast<NodeType>(1 / (static_cast<double>(h_min) / h_max));
    }

private:
    HashFunction h;
    NodeType h_min = std::numeric_limits<NodeType>::max();
    const NodeType h_max;
};

class MinSketch {
public:
    template <typename Gen>
    MinSketch(Gen& gen)
    : h(std::numeric_limits<uint32_t>::max(), gen)
    { }

    void operator() (uint32_t j) {
        h_min = std::min(h_min, h(j));
    }

    size_t count() const {
        return static_cast<size_t>((1 / (static_cast<double>(h_min) / std::numeric_limits<uint32_t>::max()) - 1));
    }

private:
    Int32ArithmeticHashFamily h;
    uint32_t h_min = std::numeric_limits<uint32_t>::max();
};

#endif //EM_CC_MINSKETCH_H
