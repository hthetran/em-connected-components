/*
 * make_consecutively_filtered_stream.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#pragma once

template <typename StreamType, typename BinaryFunctor>
class make_consecutively_filtered_stream {
public:
    using value_type = typename StreamType::value_type;

    make_consecutively_filtered_stream(StreamType& stream_, BinaryFunctor probe_, value_type sentinel_ = value_type())
     : stream(stream_),
       probe(probe_),
       sentinel(sentinel_),
       prev(sentinel),
       curr()
    {
        if (!stream.empty()) {
            curr = *stream;
        }
    }

    make_consecutively_filtered_stream() = delete;
    make_consecutively_filtered_stream(const make_consecutively_filtered_stream&) = delete;
    make_consecutively_filtered_stream& operator=(const make_consecutively_filtered_stream&) = delete;

    const value_type& operator * () const {
        return curr;
    }

    make_consecutively_filtered_stream& operator ++ () {
        prev = curr;

        if (!stream.empty())
            ++stream;

        for (; !stream.empty(); ++stream) {
            const auto next = *stream;
            if (probe(prev, next)) {
                prev = next;
                continue;
            } else {
                curr = next;
                break;
            }
        }

        return *this;
    }

    size_t size() const {
        return stream.size();
    }

    bool empty() const {
        return stream.empty();
    }

    void rewind() {
        prev = sentinel;
        stream.rewind();
        if (!stream.empty()) {
            curr = *stream;
        }
    }

private:
    StreamType& stream;
    BinaryFunctor probe;
    const value_type sentinel;
    value_type prev;
    value_type curr;
};