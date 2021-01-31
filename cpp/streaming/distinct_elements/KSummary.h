/*
 * KSummary.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#ifndef EM_CC_KSUMMARY_H
#define EM_CC_KSUMMARY_H

#include <unordered_set>

template <size_t k, typename ValueType>
class KSummary {
public:
    KSummary() = default;

    void operator() (ValueType x) {
        if (next_k.find(x) != next_k.cend()) {
            return;
        } else {
            num_misses++;
            if (next_k_min < x) {
                while (*next_k.begin() < x) {
                    next_k.erase(*next_k.begin());
                }
                if (!next_k.empty()) {
                    next_k_min = *next_k.begin();
                } else {
                    next_k_min = std::numeric_limits<ValueType>::max();
                }
            }
            if (next_k.size() < k) {
                next_k.insert(x);
            }
        }
    }

    size_t count() const {
        return num_misses;
    }
private:
    size_t num_misses = 0;
    std::unordered_set<ValueType> next_k;
    ValueType next_k_min = std::numeric_limits<ValueType>::max();
};

#endif //EM_CC_KSUMMARY_H
