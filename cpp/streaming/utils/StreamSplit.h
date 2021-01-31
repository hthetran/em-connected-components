/*
 * StreamSplit.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#pragma once

#include "../../defs.hpp"

template <typename In, typename Out, typename Callback>
class StreamSplit {
public:
    using value_type_in = typename In::value_type;
    using value_type_out = typename Out::value_type;
    using value_type = value_type_in;
    using callback_type = Callback;

private:
    enum Mode {READING, REREADING};
    Mode mode;
    In & in;
    Out & out;
    const callback_type callback;

public:
    StreamSplit(In & in_, Out & out_, const callback_type & callback_)
        : mode(READING),
          in(in_),
          out(out_),
          callback(callback_)
    { }

    const value_type & operator * () const {
        return *in;
    }

    StreamSplit & operator ++ () {
        if (mode == READING) {
            out.push(callback(*in));
        }

        ++in;

        return * this;
    }

    bool empty() const {
        return in.empty();
    }

    void rewind() {
        in.rewind();

        mode = REREADING;
    }

    size_t size() const {
        return in.size();
    }
};
