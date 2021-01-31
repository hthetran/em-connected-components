/*
 * StreamRandomNeighbour.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#pragma once

#include "../../defs.hpp"
#include "../hungdefs.hpp"
#include "WeightedCoin.h"

template <typename EdgesIn>
class StreamRandomNeighbour {
public:
    using value_type_in = typename EdgesIn::value_type;
    using value_type = value_type_in;

private:
    using em_buffer_t = stxxl::sequence<value_type>;
    using em_reader_t = typename em_buffer_t::stream;

    enum Mode {PROCESSING, REREADING};
    std::unique_ptr<em_buffer_t> em_buffer_ptr;
    std::unique_ptr<em_reader_t> em_reader_ptr;

    Mode mode;
    EdgesIn & sorted_edges;
    double p;
    WeightedCoin select;
    node_t source;
    value_type current;
    node_t counter;
    bool at_last;
    bool is_empty;
    size_t random_edges;
    node_t num_sampled_neighbours;
    node_t num_unsampled_neighbours;

    std::random_device rd;
    std::mt19937_64 gen;
    std::uniform_real_distribution<> dist;

public:
    StreamRandomNeighbour(EdgesIn & sorted_edges_, double p_) :
    mode(PROCESSING),
    sorted_edges(sorted_edges_),
    p(p_),
    select(p_),
    source(INVALID_NODE),
    current(INVALID_NODE, INVALID_NODE),
    counter(1),
    at_last(false),
    is_empty(false),
    random_edges(0),
    num_sampled_neighbours(0),
    num_unsampled_neighbours(0),
    gen(rd()),
    dist(0, 1)
    {
        em_reader_ptr.reset(nullptr);
        em_buffer_ptr.reset(new em_buffer_t(16, 16));

        if (sorted_edges_.empty()) {
            is_empty = true;
        } else {
            // TODO capsulate
            bool take_edge;
            do {
                take_edge = select(gen);
                find_random_neighbour(take_edge);
                if (is_empty) {
                    break;
                }
            } while (!take_edge);
        }
    }

    ~StreamRandomNeighbour() {
        // in this order ;)
        em_reader_ptr.reset(nullptr);
        em_buffer_ptr.reset(nullptr);
    }

    const value_type & operator * () const {
        return current;
    }

    const value_type * operator -> () const {
        return & current;
    }

    StreamRandomNeighbour & operator ++ () {
        if (mode == PROCESSING) {
            if (at_last) {
                is_empty = true;
            } else {
                bool take_edge;
                do {
                    take_edge = select(gen);
                    find_random_neighbour(take_edge);
                    if (is_empty) {
                        break;
                    }
                } while (!take_edge);
            }
        } else {
            ++(*em_reader_ptr);

            if (!(*em_reader_ptr).empty()) {
                current = (*(*em_reader_ptr));
            }
        }

        return * this;
    }

    bool empty() const {
        if (mode == PROCESSING) {
            return is_empty;
        } else {
            return (*em_reader_ptr).empty();
        }
    }

    void rewind() {
        mode = REREADING;
        em_reader_ptr.reset(new em_reader_t(*em_buffer_ptr));

        if (!(*em_reader_ptr).empty()) {
            current = (*(*em_reader_ptr));
        }
    }

    size_t size() const {
        return random_edges;
    }

    node_t get_num_sources() const {
        return num_sampled_neighbours + num_unsampled_neighbours;
    }

    node_t get_num_sampled_neighbours() const {
        return num_sampled_neighbours;
    }

    node_t get_num_unsampled_neighbours() const {
        return num_unsampled_neighbours;
    }

private:
    void find_random_neighbour(bool take_edge) {
        assert(mode == PROCESSING);

        num_sampled_neighbours += take_edge;
        num_unsampled_neighbours += !take_edge;

        // should be at first element in the stream of this source
        source = (*sorted_edges).u;

        if (take_edge) {
            counter = 1;

            // find the random neighbour
            for (; !sorted_edges.empty(); ++sorted_edges) {
                const auto edge = *sorted_edges;
                if (edge.u != source) {
                    // source node has changed
                    source = edge.u;

                    break;
                } else {
                    // source node remained the same, redraw neighbour
                    // if redraw is successful, update current random output neighbour
                    const double redraw = dist(gen);
                    if (redraw < 1. / counter) {
                        current = edge;
                    }

                    counter++;
                }
            }

            (*em_buffer_ptr).push_back(current);
            random_edges++;
        } else {
            // find next source
            for (; !sorted_edges.empty(); ++sorted_edges) {
                const auto edge = *sorted_edges;
                if (edge.u != source) break;
            }

            // if at end of stream without taking the edge mark as empty
            if (sorted_edges.empty()) {
                is_empty = true;
            }
        }

        at_last = sorted_edges.empty();
    }
};