/*
 * EdgeSequence.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#pragma once

#include "../../defs.hpp"
#include "../hungdefs.hpp"
#include <memory>
#include <stxxl/sequence>

template <size_t BlockSize = STXXL_DEFAULT_BLOCK_SIZE(edge_t)>
class BlockedEdgeSequence {
public:
    using value_type = edge_t;

    BlockedEdgeSequence() : m_edges(new stxxl::sequence<edge_t, BlockSize>()), m_output(nullptr) { }
    BlockedEdgeSequence(BlockedEdgeSequence && o) = delete;
    BlockedEdgeSequence & operator = (BlockedEdgeSequence && o) = delete;
    ~BlockedEdgeSequence() {
        m_output.reset(nullptr);
        //if (!m_edges->empty())
            m_edges.reset(nullptr);
    }

    void rewind() {
        m_output = std::make_unique<typename stxxl::sequence<edge_t, BlockSize>::stream>(m_edges->get_stream());
    }

    void push(const edge_t & edge) {
        die_unless_valid_edge(edge);
        m_edges->push_back(edge);
    }

    void push_back(const edge_t & edge) {
        m_edges->push_back(edge);
    }

    bool empty() const {
        return m_output->empty();
    }

    BlockedEdgeSequence& operator++() {
        m_output->operator++();

        return *this;
    }

    edge_t operator* () {
        return m_output->operator*();
    }

    [[nodiscard]] size_t size() const {
        return m_edges->size();
    }

    void swap(BlockedEdgeSequence& o) {
        std::swap(m_edges,  o.m_edges);
        std::swap(m_output, o.m_output);
    }

protected:
    std::unique_ptr<stxxl::sequence<edge_t, BlockSize>> m_edges;
    std::unique_ptr<typename stxxl::sequence<edge_t, BlockSize>::stream> m_output;
};

using EdgeSequence = BlockedEdgeSequence<>;