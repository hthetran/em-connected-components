#pragma once

#include "../../defs.hpp"
#include "../../util.hpp"
#include "../../simpleshiftmap.hpp"


class BoundedIntervalKruskal {

protected:
	node_t interval_min;
	node_t interval_max;
	SimpleShiftMap<node_t, node_t>& parent;
	SimpleShiftMap<node_t, unsigned> height;

public:
	static constexpr size_t MEMORY_OVERHEAD_FACTOR = 4;
	BoundedIntervalKruskal(SimpleShiftMap<node_t, node_t>& parent, node_t interval_min, node_t interval_max) :
		interval_min(interval_min),
		interval_max(interval_max),
		parent(parent),
		height(interval_min, interval_max)
	{}

	void push(edge_t e) {
		op_union(e.u, e.v);
	}

	void finalize() {
		for (node_t u = interval_min; u <= interval_max; ++u) {
			if (parent.contains(u)) {
				node_t root = op_find(u);
				parent[u] = root;
			}
		}
	}

protected:
	inline node_t op_find(node_t u) {
		// find root
		if (!parent.contains(u)) {
			parent.insert(u, u);
			height.insert(u, 0);
			return u;
		}
		node_t root = u;
		while (parent[root] != root) {
			root = parent[root];
		}

		// apply path compression
		node_t tmp;
		while (parent[u] != u) {
			tmp = parent[u];
			parent[u] = root;
			u = tmp;
		}

		return root;
	}

	inline bool op_union(const node_t& u, const node_t& v) {
		const node_t root_u = op_find(u);
		const node_t root_v = op_find(v);

		// cycle detected
		if (root_u == root_v) {
			return false;
		}

		// attach smaller to bigger tree
		if (height[root_u] < height[root_v]) {
			parent[root_u] = root_v;
		} else {
			parent[root_v] = root_u;
		}

		// increment the height of the resulting tree if they are the same
		if (height[root_u] == height[root_v]) {
			++height[root_u];
		}

		return true;
	}
};
