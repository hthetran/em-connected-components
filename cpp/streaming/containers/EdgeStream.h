/*
 * EdgeStream.h
 *
 * Copyright (C) 2016 - 2020 Manuel Penschuck,
 *                           Michael Hamann,
 *               2017 - 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#pragma once

#include "../../defs.hpp"
#include "../hungdefs.hpp"
#include <stxxl/sequence>
#include <memory>

class EdgeStream {
public:
    using value_type = edge_t;

protected:
    static_assert(std::is_unsigned_v<node_t>, "left bit shift are ub for signed type");
    static constexpr node_t kOutNodeSwitch = node_t{1} << (8*sizeof(node_t) - 1);

    using em_buffer_t = stxxl::sequence<node_t>;
    using em_reader_t = typename em_buffer_t::stream;

    std::unique_ptr<em_buffer_t> _em_buffer;
    std::unique_ptr<em_reader_t> _em_reader;

    enum Mode {WRITING, READING};
    Mode _mode;

    bool _allow_multi_edges;
    bool _allow_loops;


    // WRITING
    node_t _current_out_node;
    size_t _number_of_edges;

    size_t _number_of_selfloops;
    size_t _number_of_multiedges;

    // READING
    value_type _current;
    bool _empty;

public:
    EdgeStream(bool multi_edges = true, bool loops = true)
    : _allow_multi_edges(multi_edges)
    , _allow_loops(loops)
    , _current(MAX_EDGE)
    {clear();}

    EdgeStream(const EdgeStream &) = delete; // ; , bool multi_edges = false, bool loops = false) = delete;

    ~EdgeStream() {
        // in this order ;)
        _em_reader.reset(nullptr);
        _em_buffer.reset(nullptr);
    }

    EdgeStream(EdgeStream&&) = default;

    EdgeStream& operator=(EdgeStream&&) = default;

    // Write interface
    void push(const edge_t& edge) {
        die_unless_valid_edge(edge);
        assert(edge.u < kOutNodeSwitch);
        assert(edge.v < kOutNodeSwitch);

        assert(_mode == WRITING);

        // count selfloops and fail if they are illegal
        {
            const bool selfloop = (edge.u == edge.v);
            _number_of_selfloops += selfloop;
            assert(_allow_loops || !selfloop);
        }

        // count multiedges and fail if they are illegal
        {
            const bool multiedge = (edge == _current);
            _number_of_multiedges += multiedge;
            assert(_allow_multi_edges || !multiedge);
        }

        // ensure order
        assert(!_number_of_edges || _current <= edge);

        if (_current_out_node != edge.u) {
            _em_buffer->push_back(edge.u | kOutNodeSwitch);
            _current_out_node = edge.u;
        }

        _em_buffer->push_back(edge.v);
        _number_of_edges++;

        _current = edge;
    }

    //! see rewind
    void consume() {rewind();}

    //! switches to read mode and resets the stream
    void rewind() {
        _mode = READING;
        _em_reader.reset(new em_reader_t(*_em_buffer));
        _current = {0, 0};
        _empty = _em_reader->empty();

        if (!empty())
            ++(*this);
    }

    // returns back to writing mode on an empty stream
    void clear() {
        _mode = WRITING;
        _current_out_node = 0;
        _number_of_edges = 0;
        _number_of_multiedges = 0;
        _number_of_selfloops = 0;
        _em_reader.reset(nullptr);
        _em_buffer.reset(new em_buffer_t(16, 16));
    }

    void swap(EdgeStream& other) {
        std::swap(*this, other);
    }

    //! Number of edges available if rewind was called
    const size_t & size() const {
        return _number_of_edges;
    }

    const size_t & selfloops() const {
        return _number_of_selfloops;
    }

    const size_t & multiedges() const {
        return _number_of_multiedges;
    }

// Consume interface
    //! return true when in write mode or if edge list is empty
    bool empty() const {
        return _empty;
    }

    const value_type& operator*() const {
        assert(READING == _mode);
        return _current;
    }

    const value_type* operator->() const {
        assert(READING == _mode);
        return &_current;
    }


    EdgeStream& operator++() {
        assert(READING == _mode);
        assert(!_empty);

        em_reader_t& reader = *_em_reader;

        // handle end of stream
        _empty = reader.empty();
        if (_empty)
            return *this;

        if (*reader >= kOutNodeSwitch) {
            _current.u = *reader & ~kOutNodeSwitch;
            ++reader;
        }

        assert(!reader.empty());
        _current.v = *reader;
        assert(_current.v < kOutNodeSwitch);
        ++reader;

        die_unless_valid_edge(_current);

        return *this;
    }
};
