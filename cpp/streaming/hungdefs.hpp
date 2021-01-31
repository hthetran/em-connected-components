#pragma once

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdint.h>

constexpr bool debug = true;
constexpr bool debug_sorter = true && debug;
#define STRING(s) #s
#define LOG_SORTER(SORTER) /*\
    if (debug_sorter) { \
	    std::cout << STRING(SORTER); \
        for (; !SORTER.empty(); ++SORTER) { \
	        std::cout << *SORTER; \
        } \
        SORTER.rewind(); \
        std::cout << std::endl; \
    }
                           */
#define LOG_REWINDSTREAM(STREAM) \
    std::cout << STRING(STREAM) << std::endl; \
    for (; !STREAM.empty(); ++STREAM) { \
        std::cout << *STREAM << std::endl; \
    } \
    STREAM.rewind();

constexpr node_t INVALID_NODE = std::numeric_limits<node_t>::max();

struct node_less_cmp {
    bool operator() (const node_t & a, const node_t & b) const {
        return a < b;
    }

    node_t min_value() const {
        return MIN_NODE;
    }

    node_t max_value() const {
        return MAX_NODE;
    }
};

struct edge_source_lessequal {
    bool operator ()(const edge_t & a, const node_t & b) const {
        return a.u <= b;
    }
};

struct edge_source_equal {
    bool operator ()(const edge_t & a, const node_t & b) const {
        return a.u == b;
    }
};

#define DECL_EDGE_MIN(MINU, MINV) \
    edge_t min_value() const { \
        return edge_t{MINU, MINV}; \
    }

#define DECL_EDGE_MAX(MAXU, MAXV) \
    edge_t max_value() const { \
        return edge_t{MAXU, MAXV}; \
    }

struct edge_less_cmp {
    bool operator() (const edge_t& a, const edge_t& b) const {
        return (a.u < b.u) || (a.u == b.u && a.v < b.v);
    }
    DECL_EDGE_MIN(MIN_NODE, MIN_NODE);
    DECL_EDGE_MAX(MAX_NODE, MAX_NODE);
};

struct edge_reverse_less_cmp {
    bool operator() (const edge_t& a, const edge_t& b) const {
        return (a.v < b.v) || (a.v == b.v && a.u < b.u);
    }
    DECL_EDGE_MIN(MIN_NODE, MIN_NODE);
    DECL_EDGE_MAX(MAX_NODE, MAX_NODE);
};

struct edge_less_ordered_cmp {
    bool operator() (const edge_t & a, const edge_t & b) const {
        const auto a1 = std::min(a.u, a.v);
        const auto a2 = std::max(a.u, a.v);
        const auto b1 = std::min(b.u, b.v);
        const auto b2 = std::max(b.u, b.v);

        if (a1 < b1)
            return true;
        if (a1 > b1)
            return false;
        return a2 < b2;
    }
    DECL_EDGE_MIN(MIN_NODE, MIN_NODE);
    DECL_EDGE_MAX(MAX_NODE, MAX_NODE);
};

struct edge_greater_ordered_cmp {
    bool operator() (const edge_t & a, const edge_t & b) const {
        const auto a1 = std::min(a.u, a.v);
        const auto a2 = std::max(a.u, a.v);
        const auto b1 = std::min(b.u, b.v);
        const auto b2 = std::max(b.u, b.v);

        if (a1 > b1)
            return true;
        if (a1 < b1)
            return false;
        return a2 > b2;
    }
    DECL_EDGE_MIN(MAX_NODE, MAX_NODE);
    DECL_EDGE_MAX(MIN_NODE,MIN_NODE);
};


template <typename T>
class loaded_edge_t {
public:
    node_t u{};
    node_t v{};
    T load;

    loaded_edge_t() = default;
};

using ranked_edge_t = loaded_edge_t<uint64_t>;

inline std::ostream & operator << (std::ostream & os, const ranked_edge_t & e) {
    os << "(" << e.u << ", " << e.v << "; " << e.load << ")";
    return os;
}

struct ranked_edge_load_edge_less_cmp {
    bool operator() (const ranked_edge_t & a, const ranked_edge_t & b) const {
        return (a.load < b.load) || (a.load == b.load && ((a.u < b.u) || (a.u == b.u && a.v < b.v)));
    }

    ranked_edge_t min_value() const {
        return ranked_edge_t{MIN_NODE, MIN_NODE, MIN_NODE};
    }

    ranked_edge_t max_value() const {
        return ranked_edge_t{MAX_NODE, MAX_NODE, MAX_NODE};
    }
};

using edge_loaded_edge_t = loaded_edge_t<edge_t>;

inline std::ostream & operator << (std::ostream & os, const edge_loaded_edge_t & e) {
    os << "(" << e.u << ", " << e.v << "; " << ")";
    return os;
}

template <typename T>
class loaded_node_t {
public:
    node_t node{};
    T load;

    loaded_node_t() = default;

    bool operator == (const loaded_node_t & o) const {
        return node == o.node && load == o.load;
    }
};

template <typename T>
inline std::ostream & operator << (std::ostream & os, const loaded_node_t<T> & e) {
    os << "(" << e.node << "; " << e.load << ")";
    return os;
}

using node_pos_t = loaded_node_t<size_t>;

struct node_pos_less_cmp {
    bool operator() (const node_pos_t & a, const node_pos_t & b) const {
        return (a.node < b.node) || (a.node == b.node && a.load < b.load);
    }

    node_pos_t min_value() const {
        return node_pos_t{MIN_NODE, MIN_NODE};
    }

    node_pos_t max_value() const {
        return node_pos_t{MAX_NODE, MAX_NODE};
    }
};

using node_component_t = loaded_node_t<node_t>;

struct node_component_cc_node_less_cmp {
    bool operator() (const node_component_t & a, const node_component_t & b) const {
        return (a.load < b.load) || (a.load == b.load && a.node < b.node);
    }

    node_component_t min_value() const {
        return {MIN_NODE, MIN_NODE};
    }

    node_component_t max_value() const {
        return {MAX_NODE, MAX_NODE};
    }
};

struct node_component_node_cc_less_cmp {
    bool operator() (const node_component_t& a, const node_component_t& b) const {
        return (a.node < b.node) || (a.node == b.node && a.load < b.load);
    }

    node_component_t min_value() const {
        return {MIN_NODE, MIN_NODE};
    }

    node_component_t max_value() const {
        return {MAX_NODE, MAX_NODE};
    }
};
