/*
 * Int32ArithmeticHashFamily.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#ifndef EM_CC_INT32ARITHMETICHASHFAMILY_H
#define EM_CC_INT32ARITHMETICHASHFAMILY_H

class Int32ArithmeticHashFamily {
public:
    template <typename Gen>
    Int32ArithmeticHashFamily(uint32_t k_, Gen& gen)
     : unif(0, std::numeric_limits<uint32_t>::max()),
       k(k_),
       a(unif(gen)),
       b(unif(gen))
    { }

    uint32_t operator()(uint32_t x) const {
        const uint32_t f_ab_x = a * x + b;
        return f_ab_x & k;
    }

private:
    std::uniform_int_distribution<uint32_t> unif;
    const uint32_t k;
    const uint32_t a;
    const uint32_t b;
};

#endif //EM_CC_INT32ARITHMETICHASHFAMILY_H
