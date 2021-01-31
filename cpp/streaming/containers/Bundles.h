/*
 * Bundles.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#ifndef EM_CC_BUNDLES_H
#define EM_CC_BUNDLES_H

// TODO: put this somewhere else (and not as macro)
#define DIV_CEIL(x, y) ((x)/(y) + ((x)%(y) != 0))

#include <cassert>
#include "../../defs.hpp"
#include "../containers/EdgeSequence.h"

class EquiRangedBundles {
public:
	static constexpr size_t BUNDLE_BLOCK_SIZE = 512*1024;
	using bundle_t = BlockedEdgeSequence<BUNDLE_BLOCK_SIZE>;
	using boundary_t = node_t;

private:
	const node_t bundle_width;
	std::vector<bundle_t> bundles_vector;

public:
	EquiRangedBundles(node_t max_id, size_t num_bundles)
		: bundle_width(max_id / num_bundles),
		  bundles_vector(2 * DIV_CEIL(max_id, bundle_width))
	{}

	~EquiRangedBundles() = default;

	bundle_t& get_intrabundle_edges(size_t bundle_id) {
		return bundles_vector[2*bundle_id];
	}

	bundle_t& get_interbundle_edges(size_t bundle_id) {
		return bundles_vector[2*bundle_id + 1];
	}

	[[nodiscard]] size_t get_bundle(const node_t u) const {
		const auto bundle_id = (u-1) / bundle_width;
		assert(lower_boundary(bundle_id) <= u && u <= upper_boundary(bundle_id));
		assert(bundle_id < num_bundles());
		return bundle_id;
	}

	[[nodiscard]] node_t lower_boundary(size_t bundle_id) const {
		return bundle_id * bundle_width + 1;
	}

	[[nodiscard]] node_t upper_boundary(size_t bundle_id) const {
		return (bundle_id+1) * bundle_width;
	}

	[[nodiscard]] size_t width(size_t bundle_id) const {
		// taking id in case we want variable-width bundles later
		tlx::unused(bundle_id);
		return bundle_width;
	}

	void push(const edge_t & edge) {
		bundles_vector[2*get_bundle(edge.u) + (get_bundle(edge.u) != get_bundle(edge.v))].push(edge);
	}

	[[nodiscard]] size_t size_lower(size_t bundle_id) const {
		return bundles_vector[2*bundle_id].size();
	}

	[[nodiscard]] size_t size_upper(size_t bundle_id) const {
		return bundles_vector[2*bundle_id + 1].size();
	}

	template <typename PushContainer>
	void push_into(PushContainer& kruskal, size_t bundle_id) {
		bundle_t& subgraph = get_intrabundle_edges(bundle_id);
		subgraph.rewind();
		for (; !subgraph.empty(); ++subgraph) {
			const auto edge = *subgraph;
			kruskal.push(edge);
		}
	}

	[[nodiscard]] size_t num_bundles() const {
		return bundles_vector.size() / 2;
	}
};

#endif //EM_CC_BUNDLES_H
