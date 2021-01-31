/*
 * ApplyMedians.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#ifndef EM_CC_APPLYMEDIANS_H
#define EM_CC_APPLYMEDIANS_H

template <size_t n, typename Counter>
class ApplyMedians {
public:
    template <typename Gen>
    ApplyMedians(Gen& gen) {
        for (size_t i = 0; i < n; ++i) {
            counts.emplace_back(gen);
        }
    }

    template <typename Gen, typename NodeType>
    ApplyMedians(Gen& gen, NodeType& max_node) {
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
        std::vector<size_t> sizes;
        sizes.reserve(n);
        for (auto& entry : counts) {
            sizes.push_back(entry.count());
        }

        std::nth_element(sizes.begin(), sizes.begin() + n / 2, sizes.end());
        return sizes[n / 2];
    }

private:
    std::vector<Counter> counts;
};

#endif //EM_CC_APPLYMEDIANS_H
