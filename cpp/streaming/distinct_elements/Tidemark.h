/*
 * Tidemark.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#ifndef EM_CC_TIDEMARK_H
#define EM_CC_TIDEMARK_H

#include <cmath>
#include <limits>
#include <tlx/math/ctz.hpp>
#include "../utils/Int32ArithmeticHashFamily.h"

class Tidemark {
public:
    template <typename Gen>
    Tidemark(Gen& gen)
     : h(std::numeric_limits<uint32_t>::max(), gen),
       z(0)
     { }

     void operator() (uint32_t j) {
         z = std::max(static_cast<size_t>(tlx::ctz(h(j))), z);
     }

     size_t count() const {
         return (1<<z)*std::sqrt(2);
     }

private:
    Int32ArithmeticHashFamily h;
    size_t z;
};

#endif //EM_CC_TIDEMARK_H
