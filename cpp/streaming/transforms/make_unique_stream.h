/*
 * make_unique_stream.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#pragma once

template <typename StreamType>
class make_unique_stream {
public:
    using value_type = typename StreamType::value_type;

    make_unique_stream(StreamType& stream_, value_type sentinel_ = value_type())
     : stream(stream_),
       sentinel(sentinel_),
       prev(sentinel),
       curr()
    {
        if (!stream.empty()) {
            curr = *stream;
        }
    }

    make_unique_stream() = delete;
    make_unique_stream(const make_unique_stream&) = delete;
    make_unique_stream& operator=(const make_unique_stream&) = delete;

    const value_type& operator * () const {
        return curr;
    }

    make_unique_stream& operator ++ () {
        prev = curr;
        for (; !stream.empty(); ++stream) {
            const auto next = *stream;
            if (prev == next)
                continue;
            else {
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
    const value_type sentinel;
    value_type prev;
    value_type curr;
};