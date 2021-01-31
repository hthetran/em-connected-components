/*
 * ApplyMeans.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#ifndef EM_CC_APPLYMEANS_H
#define EM_CC_APPLYMEANS_H

template <size_t n, typename Counter>
class ApplyMeans {
public:
    template <typename Gen>
    ApplyMeans(Gen& gen) {
        for (size_t i = 0; i < n; ++i) {
            counts.emplace_back(gen);
        }
    }

    template <typename Gen, typename NodeType>
    ApplyMeans(Gen& gen, NodeType& max_node) {
        for (size_t i = 0; i < n; ++i) {
            counts.emplace_back(gen, max_node);
        }
    }

    template <typename ValueType>
    void operator() (ValueType x) {
        for (auto& entry : counts) {
            entry(x);
        }
    }

    size_t count() const {
        size_t sum = 0;
        for (auto& entry : counts) {
            sum += entry.count();
        }
        return sum / n;
    }

private:
    std::vector<Counter> counts;
};

#endif //EM_CC_APPLYMEDIANS_H
