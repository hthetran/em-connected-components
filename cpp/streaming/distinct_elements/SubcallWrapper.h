/*
 * SubcallWrapper.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#ifndef EM_CC_SUBCALLWRAPPER_H
#define EM_CC_SUBCALLWRAPPER_H

template <typename CountDistinctClass>
class SubcallWrapper {
public:
    SubcallWrapper(CountDistinctClass& left_, CountDistinctClass& right_, CountDistinctClass& both_) {
     : left(left_),
       right(right_),
       both(both_)
    { }

    template <typename ValueType>
    void add_left(ValueType& x) {
        left(x);
    }

    template <typename ValueType>
    void add_right(ValueType& x) {
        right(x);   
    }

    template <typename ValueType>
    void add_both(ValueType& x) {
        both(x);
    }

    size_t count_left() const {
        return left.count();
    }

    size_t count_right() const {
        return right.count();
    }

    size_t count_both() const {
        return both.count();
    }    

private:
    CountDistinctClass& left;
    CountDistinctClass& right;
    CountDistinctClass& both;
};

#endif //EM_CC_SUBCALLWRAPPER_H
