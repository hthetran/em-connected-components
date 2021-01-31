#pragma once

#include <algorithm>

#include <stxxl/vector>
#ifdef ZERO_NODE_ASSERT
#include <tlx/die.hpp>
#endif

using node_t = uint64_t;
constexpr size_t bytes_per_edge = sizeof(node_t)*2;

/// Types for scales
template <typename T>
struct Scale {
    static constexpr T K = static_cast<T>(1000LLU);   ///< Kilo (base 10)
    static constexpr T M = K * K;                     ///< Mega (base 10)
    static constexpr T G = K * K * K;                 ///< Giga (base 10)
    static constexpr T P = K * K * K * K;             ///< Peta (base 10)

    static constexpr T Ki = static_cast<T>(1024LLU);  ///< Kilo (base 2)
    static constexpr T Mi = Ki* Ki;                   ///< Mega (base 2)
    static constexpr T Gi = Ki* Ki* Ki;               ///< Giga (base 2)
    static constexpr T Pi = Ki* Ki* Ki* Ki;           ///< Peta (base 2)
};
using IntScale = Scale<int64_t>;
using UIntScale = Scale<uint64_t>;

constexpr size_t INTERNAL_SORT_MEM = 512 * UIntScale::Mi;
constexpr size_t INTERNAL_PQ_MEM = INTERNAL_SORT_MEM;
constexpr size_t SORTER_MEM = INTERNAL_SORT_MEM;
constexpr size_t PQ_POOL_MEM = 128 * UIntScale::Mi;
constexpr size_t MAX_PQ_SIZE = UIntScale::Gi; // is multiplied by 1024 according to docs

class edge_t {
public:
	node_t u;
	node_t v;
	bool operator== (const edge_t& other) const {
		return u==other.u && v==other.v;
	};
	bool operator!= (const edge_t& other) const {
		return u!=other.u || v!=other.v;
	};
    bool operator <= (const edge_t& other) const {
        return (u < other.u) || (u == other.u && v <= other.v);
    }
	bool self_loop() const {
		return u==v;
	};
	void orient_smaller_to_larger() {
		if (u>v) {
			std::swap(u,v);
		}
	}
	edge_t normalized() const {
		return edge_t{std::min(u,v), std::max(u,v)};
	}
	edge_t() = default;
	edge_t(const node_t _u, const node_t _v) : u(_u), v(_v) { };
};

using em_edge_vector = stxxl::vector<edge_t>;
using im_edge_vector = std::vector<edge_t>;
using em_node_vector = stxxl::vector<node_t>;
using im_node_vector = std::vector<node_t>;

using em_mapping = stxxl::vector<edge_t>;

constexpr node_t MIN_NODE = std::numeric_limits<node_t>::min();
constexpr node_t MAX_NODE = std::numeric_limits<node_t>::max();
#define MIN_EDGE edge_t{MIN_NODE, MIN_NODE}
#define MAX_EDGE edge_t{MAX_NODE, MAX_NODE}
// for regular sorting
struct node_lt_ordering {
	bool operator() (node_t u, node_t v) const {
		return u < v;
	}
	node_t min_value() const {
		return MIN_NODE;
	}
	node_t max_value() const {
		return MAX_NODE;
	}
};
// for regular sorting
struct edge_lt_ordering {
	bool operator() (edge_t a, edge_t b) const {
		return a.u < b.u || (a.u == b.u && a.v < b.v);
	}
	edge_t min_value() const {
		return MIN_EDGE;
	}
	edge_t max_value() const {
		return MAX_EDGE;
	}
};
// for min-priority queue
struct edge_gt_ordering {
	bool operator() (edge_t a, edge_t b) const {
		return a.u > b.u || (a.u == b.u && a.v > b.v);
	}
	edge_t min_value() const {
		return MAX_EDGE;
	}
	edge_t max_value() const {
		return MIN_EDGE;
	}
};
// for sorting edges by target
struct edge_target_lt_ordering {
	bool operator() (edge_t a, edge_t b) const {
		return a.v < b.v || (a.v == b.v && a.u < b.u);
	}
	edge_t min_value() const {
		return MIN_EDGE;
	}
	edge_t max_value() const {
		return MAX_EDGE;
	}
};
// for PQ getting minimum u with maximum v out
struct edge_gt_lt_ordering {
	bool operator() (edge_t a, edge_t b) const {
		return a.u > b.u || (a.u == b.u && a.v < b.v);
	}
	edge_t min_value() const {
		return edge_t{MAX_NODE, MIN_NODE};
	}
	edge_t max_value() const {
		return edge_t{MIN_NODE, MAX_NODE};
	}
};

#ifdef ZERO_NODE_ASSERT
inline void die_unless_valid_edge(const edge_t& e) {
    die_unless(e.u > 0);
    die_unless(e.v > 0);
}
#else
inline void die_unless_valid_edge(const edge_t&) {}
#endif
