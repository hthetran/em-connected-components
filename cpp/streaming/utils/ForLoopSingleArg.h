/*
 * ForLoopSingleArg.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#ifndef EM_CC_FORLOOPSINGLEARG_H
#define EM_CC_FORLOOPSINGLEARG_H

#include <iostream>
#include <array>

template<size_t c>
struct ForLoopSingleArg {
    template<template <size_t> class Func, typename Input>
    static void iterate(Input& input) {
        Func<c>()(input);
        ForLoopSingleArg<c-1>::template iterate<Func>(input);
    }
};

template<>
struct ForLoopSingleArg<1> {
    template<template <size_t> class Func, typename Input>
    static void iterate(Input& input) {
        Func<1>()(input);
    }
};

#endif //EM_CC_FORLOOPSINGLEARG_H
