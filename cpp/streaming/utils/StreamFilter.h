/*
 * StreamFilter.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#pragma once

template <typename In, typename Hit, typename LessEqualPred, typename EqualPred>
class StreamHitFilter {
public:
    using value_type_in = typename In::value_type;
    using value_type = value_type_in;

private:
    In & in;
    Hit & hit;
    const LessEqualPred less_equal;
    const EqualPred equal;
    size_t num_edges_output;

public:
    StreamHitFilter(In & in_, Hit & hit_, const LessEqualPred & less_equal_, const EqualPred & equal_)
        : in(in_),
          hit(hit_),
          less_equal(less_equal_),
          equal(equal_),
          num_edges_output(0)
    {
        next_element();
    }

    const value_type & operator * () const {
        return *in;
    }

    StreamHitFilter & operator ++ () {
        ++in;
        next_element();
        num_edges_output++;

        return * this;
    }

    bool empty() const {
        return in.empty();
    }

    void rewind() {
        in.rewind();
        hit.rewind();
        next_element();
        num_edges_output = 0;
    }

    size_t size() {
        return num_edges_output;
    }

private:
    void advance_hits() {
        for (; !hit.empty(); ++hit) {
            if (less_equal(*in, *hit)) {
                break;
            }
        }
    }

    void next_element() {
        for (; !in.empty(); ++in) {
            advance_hits();

            if (!hit.empty()) {
                if (!equal(*in, *hit)) {
                    break;
                } else {
                    continue;
                }
            } else {
                break;
            }
        }
    }
};