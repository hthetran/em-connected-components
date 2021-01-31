/*
 * StreamMerge.h
 *
 * Copyright (C) 2019 - 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#pragma once

template <typename StreamA, typename StreamB, typename Selector, typename Extractor>
class TwoWayStreamMerge {
public:
    using value_type = typename Extractor::value_type;
    using streampair_type = std::pair<StreamA &, StreamB &>;

private:
    streampair_type in;
    value_type current{};
    bool stream_indicator = false;
    const Selector & selecter;
    const Extractor & extracter;

    void choose() {
        if (in.first.empty()) {
            stream_indicator = true;
            current = extracter.extract(*(in.second), stream_indicator);
        }

        if (in.second.empty()) {
            stream_indicator = false;
            current = extracter.extract(*(in.first), stream_indicator);
        }

        if (!in.first.empty() && !in.second.empty()) {
            const bool selection = selecter.select(*(in.first), *(in.second));
            stream_indicator = selection;

            if (!selection)
                current = extracter.extract(*(in.first), stream_indicator);
            else
                current = extracter.extract(*(in.second), stream_indicator);
        }
    }

public:
    TwoWayStreamMerge(StreamA & in_a, StreamB & in_b, const Selector & selecter_, const Extractor & extracter_)
    : in(in_a, in_b), selecter(selecter_), extracter(extracter_) {
        if (!empty())
            choose();
    }

    const value_type & operator * () const {
        return current;
    }

    TwoWayStreamMerge & operator ++ () {
        assert(!empty());

        if (!stream_indicator)
            ++in.first;
        else
            ++in.second;

        if (!empty())
            choose();

        return *this;
    }

    bool empty() const {
        return in.first.empty() && in.second.empty();
    }

    size_t size() const {
        return in.first.size() + in.second.size();
    }
};