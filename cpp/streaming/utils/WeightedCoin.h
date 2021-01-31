/*
 * WeightedCoin.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#pragma once

class WeightedCoin {
public:
    explicit WeightedCoin(double sampling_prob_) :
    dist(sampling_prob_),
    sampling_prob(sampling_prob_)
    { }

    template <typename Gen>
    bool operator() (Gen& gen) {
        return dist(gen);
    }

    [[nodiscard]] double get_probability() const {
        return sampling_prob;
    }

    void reset() {
        dist.reset();
    }

private:
    std::bernoulli_distribution dist;
    const double sampling_prob;
};