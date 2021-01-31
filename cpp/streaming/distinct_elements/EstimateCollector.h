/*
 * EstimateCollector.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#ifndef EM_CC_ESTIMATECOLLECTOR_H
#define EM_CC_ESTIMATECOLLECTOR_H

#include <tuple>

template <typename Estimators...>
class EstimateCollector {
public:
    EstimateCollector(Estimators& estimators_...) 
     : estimators(estimators_)  
    { }

private:
    std::tuple<Estimators&...> estimators;
};

#endif //EM_CC_ESTIMATECOLLECTOR_H
