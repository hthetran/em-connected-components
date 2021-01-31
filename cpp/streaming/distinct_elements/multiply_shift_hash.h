/*
 * multiply_shift_hash.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#ifndef EM_CC_MULTIPLY_SHIFT_HASH_H
#define EM_CC_MULTIPLY_SHIFT_HASH_H

#include <limits>
#include <random>
#include <stdint.h>
#include <tlx/math/integer_log2.hpp>

class MultiplyShiftHash32Bit {
public:
    template <typename Gen>
    MultiplyShiftHash32Bit(Gen& gen)
        : dist(std::numeric_limits<uint64_t>::min(), std::numeric_limits<uint64_t>::max()),
          h_a(dist(gen)),
          h_b(dist(gen)),
          h_l(32)
    { }

    template <typename Gen>
    MultiplyShiftHash32Bit(Gen& gen, uint64_t max_value)
        : dist(std::numeric_limits<uint64_t>::min(), std::numeric_limits<uint64_t>::max()),
          h_a(dist(gen)),
          h_b(dist(gen)),
          h_l(get_bits(max_value))
    { }

    uint32_t operator() (uint32_t x) {
        return (h_a * x + h_b) >> (64 - h_l);
    }

private:
    std::uniform_int_distribution<uint64_t> dist;
    const uint64_t h_a;
    const uint64_t h_b;
    const uint32_t h_l;

    uint32_t get_bits(uint32_t max_value) {
        return tlx::integer_log2_ceil(max_value);
    }
};

class MultiplyShiftHash64Bit {
public:
    template <typename Gen>
    MultiplyShiftHash64Bit(Gen& gen)
     : dist(std::numeric_limits<uint64_t>::min(), std::numeric_limits<uint64_t>::max()),
       h0_a1(dist(gen)),
       h0_a2(dist(gen)),
       h0_b(dist(gen)),
       h0_l(32),
       h1_a1(dist(gen)),
       h1_a2(dist(gen)),
       h1_b(dist(gen)),
       h1_l(32)
    { }

    template <typename Gen>
    MultiplyShiftHash64Bit(Gen& gen, uint64_t max_value)
        : dist(std::numeric_limits<uint64_t>::min(), std::numeric_limits<uint64_t>::max()),
          h0_a1(dist(gen)),
          h0_a2(dist(gen)),
          h0_b(dist(gen)),
          h0_l(get_bits(max_value).first),
          h1_a1(dist(gen)),
          h1_a2(dist(gen)),
          h1_b(dist(gen)),
          h1_l(get_bits(max_value).second)
    { }

    uint64_t operator() (uint64_t x) {
        return (static_cast<uint64_t>(h1(x)) << 32) + h0(x);
    }

private:
    std::uniform_int_distribution<uint64_t> dist;
    const uint64_t h0_a1;
    const uint64_t h0_a2;
    const uint64_t h0_b;
    const uint64_t h0_l;
    const uint64_t h1_a1;
    const uint64_t h1_a2;
    const uint64_t h1_b;
    const uint64_t h1_l;

    inline uint32_t h0(uint64_t x) const {
        return ((h0_a1 + x) * (h0_a2 + (x >> 32)) + h0_b) >> (64 - h0_l);
    }

    inline uint32_t h1(uint64_t x) const {
        return ((h1_a1 + x) * (h1_a2 + (x >> 32)) + h1_b) >> (64 - h1_l);
    }

    std::pair<uint64_t, uint64_t> get_bits(uint64_t max_value) {
        const uint64_t log2value = tlx::integer_log2_ceil(max_value);
        return std::make_pair((log2value > 32 ? 32 : log2value), (log2value > 32 ? log2value - 32 : 0));
    }
};

#endif //EM_CC_MULTIPLY_SHIFT_HASH_H