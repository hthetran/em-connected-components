/*
 * BaseKruskal.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#pragma once

#include "../../robin_hood.h"
#include "../../defs.hpp"
#include "../../util.hpp"
#include "../hungdefs.hpp"

#define im_node_map robin_hood::unordered_map
#define simple_node_map std::vector

class BaseKruskal {
public:
	static constexpr size_t MEMORY_OVERHEAD_FACTOR = 8;

    BaseKruskal()
    : _next_node(0),
      _id_map(),
      _reverse_map(),
      _parent(),
      _height()
    { }

    node_t get_num_nodes() const {
        return _next_node;
    }

    node_t get_num_unions() const {
        return _num_unions;
    }

    node_t get_num_ccs() const {
        return _next_node - _num_unions;
    }

protected:
    node_t _num_unions = 0;
    node_t _next_node;
    im_node_map<node_t, node_t> _id_map;
    simple_node_map<node_t> _reverse_map;
    simple_node_map<node_t> _parent;
    simple_node_map<uint8_t> _height;

    template <typename ComponentsSorter>
    inline void process_output(ComponentsSorter& components) {
        for (node_t i = 0; i < _reverse_map.size(); ++i) {
            const node_t & u = _reverse_map[i];
            const node_t & j = op_find(i);
            const node_t & v = _reverse_map[j];
            components.push(typename ComponentsSorter::value_type{u, v});
        }
    }

    inline node_t use_map(node_t u) {
        auto lookup = _id_map.find(u);
        if (lookup == _id_map.end()) {
            _id_map[u] = _next_node;
            _reverse_map.push_back(u);
            _parent.push_back(_next_node);
            _height.push_back(0);
            _next_node++;
            return _next_node-1;
        } else {
            return lookup->second;
        }
    }

    inline node_t op_find(node_t u) {
        // find root
        node_t root = u;
        while (_parent[root] != root) {
            root = _parent[root];
        }

        // apply path compression
        node_t tmp;
        while (_parent[u] != u) {
            tmp = _parent[u];
            _parent[u] = root;
            u = tmp;
        }

        return root;
    }

    inline bool op_union(node_t u, node_t v) {
        const node_t root_u = op_find(u);
        const node_t root_v = op_find(v);

        // cycle detected
        if (root_u == root_v) return false;

        // attach smaller to bigger tree
        if (_height[root_u] < _height[root_v]) {
            _parent[root_u] = root_v;
        } else {
            _parent[root_v] = root_u;
        }

        // increment the height of the resulting tree if they are the same
        if (_height[root_u] == _height[root_v]) {
            ++_height[root_u];
        }

        return true;
    }
};
