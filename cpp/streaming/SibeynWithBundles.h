/*
 * SibeynWithBundles.h
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#ifndef EM_CC_SIBEYNWITHBUNDLES_H
#define EM_CC_SIBEYNWITHBUNDLES_H

#include <foxxll/mng/read_write_pool.hpp>
#include <stxxl/priority_queue>
#include <stxxl/sorter>

#include "../defs.hpp"
#include "hungdefs.hpp"
#include "basecase/BoundedIntervalKruskal.hpp"
#include "containers/EdgeStream.h"
#include "../simpleshiftmap.hpp"
#include "transforms/make_unique_stream.h"


template <typename BundlesType>
class SibeynWithBundles {
	using pq_type = stxxl::PRIORITY_QUEUE_GENERATOR<edge_t, edge_lt_ordering, INTERNAL_PQ_MEM, MAX_PQ_SIZE>::result;
	using pq_block_type = pq_type::block_type;
	using pq_pool_type = foxxll::read_write_pool<pq_block_type>;
	using value_type = edge_t;

protected:
	EdgeStream& edges;
	BundlesType bundles;
	pq_pool_type tree_pq_pool;
	pq_type tree_pq;
	value_type current_out;
	bool is_last = false;
	bool is_empty = false;
	bool minimize_interbundle_edges;

public:
	SibeynWithBundles(EdgeStream& edges, node_t max_id, size_t internal_memory_bytes, size_t num_bundles, bool minimize_interbundle_edges_ = true)
		: edges(edges),
		  bundles(max_id, num_bundles),
		  //std::min(max_id, // in extreme tests, can go down to singleton buckets
		  //                 DIV_CEIL(SEMIEXT_OVERHEAD_FACTOR*max_id*sizeof(node_t), internal_memory_bytes))),
		  tree_pq_pool(PQ_POOL_MEM / (2 * pq_block_type::raw_size), PQ_POOL_MEM / (2 * pq_block_type::raw_size)),
		  tree_pq(tree_pq_pool),
		  minimize_interbundle_edges(minimize_interbundle_edges_)
	{
		std::cout << "Number of bundles: " << bundles.num_bundles() << std::endl;
		std::cout << "Bundle width (#nodes): " << bundles.width(0) << std::endl;
		std::cout << "Memory budget fits (#nodes, not considering overhead): " << internal_memory_bytes/sizeof(node_t) << std::endl;
		for (; !edges.empty(); ++edges) {
			const auto edge = *edges;
			assert(edge.u <= edge.v);
			bundles.push(edge);
		}

		for (size_t i = 0; i < bundles.num_bundles(); ++i) {
			process_bundle(i);
		}

		this->operator++();
	}

	[[nodiscard]] bool empty() {
		return is_empty;
	}

	value_type operator* () const {
		return current_out;
	}

	SibeynWithBundles& operator++ () {
		if (is_last && tree_pq.empty()) {
			is_empty = true;
			return *this;
		}

		// top most message is correct
		current_out = tree_pq.top();
		const auto source = current_out.u;
		const auto target = current_out.v;

		// forward to neighbors itself, propagate maximum id down
		while (!tree_pq.empty() && (tree_pq.top().u == source)) {
			const auto current_tree_top = tree_pq.top();
			tree_pq.pop();
			if (current_tree_top.u > current_tree_top.v) {
				tree_pq.push(edge_t{current_tree_top.v, target});
			}
		}
		if (tree_pq.empty() && !is_empty)
			is_last = true;

		return *this;
	}

	~SibeynWithBundles() = default;

protected:
	void process_bundle(size_t bundle_id) {
		// (1) solve "lower" part
		// map node -> star center (within bundle)
		SimpleShiftMap<node_t, node_t> components(bundles.lower_boundary(bundle_id), bundles.upper_boundary(bundle_id));
		BoundedIntervalKruskal kruskal(components, bundles.lower_boundary(bundle_id), bundles.upper_boundary(bundle_id));
		bundles.push_into(kruskal, bundle_id);

		// map star center -> farthest neighbor (inter/intra bundle)
		SimpleShiftMap<node_t, node_t> maximas(bundles.lower_boundary(bundle_id), bundles.upper_boundary(bundle_id));

		kruskal.finalize();

		if (bundle_id < bundles.num_bundles() - 1) { // not at last bundle
			auto& upper_part = bundles.get_interbundle_edges(bundle_id);
			upper_part.rewind();

			// (2abcd) relink "upper" part
			// --- iterate through "upper" part of the bundle and update its components' maximum
			for (; !upper_part.empty(); ++upper_part) {
				const edge_t& edge = *upper_part;
				assert(bundles.lower_boundary(bundle_id) <= edge.u && edge.u <= bundles.upper_boundary(bundle_id));
				assert(bundles.upper_boundary(bundle_id) < edge.v);
				// set maximum
				const node_t& comp = components.get(edge.u, edge.u);
				maximas.insert_or_max(comp, edge.v);
			}

			// --- iterate through "upper" part and generate new messages
			upper_part.rewind();
			if (minimize_interbundle_edges && upper_part.size() > 1) {
				// TODO: streamify this construction more
				// group by target, want sources in increasing order
				using signal_sorter = stxxl::sorter<edge_t, edge_target_lt_ordering>;
				signal_sorter sorted_signals(edge_target_lt_ordering(), SORTER_MEM);
				for (; !upper_part.empty(); ++upper_part) {
					const edge_t& edge = *upper_part;
					const node_t comp = components.get(edge.u, edge.u);
					assert(edge.v <= maximas[comp]);
					const node_t max = maximas[comp];
					if (edge.v != max) {
						sorted_signals.push(edge_t{edge.v, max});
					}
				}
				if (sorted_signals.size() == 0) {
					goto output_tree_edges;
				}
				sorted_signals.sort();
				make_unique_stream<signal_sorter> unique_signals(sorted_signals);
				// TODO: handle case with 0-1 signals
				edge_t e = *unique_signals;
				node_t prev_source = e.u;
				size_t prev_source_bundle = bundles.get_bundle(e.u);
				node_t prev_max = e.v;
				++unique_signals;
				for (; !unique_signals.empty(); ++unique_signals) {
					e = *unique_signals;
					const size_t source_bundle = bundles.get_bundle(e.u);
					if (e.v == prev_max) { // two signals pointing to same max
						if (source_bundle == prev_source_bundle) { // and they are in the same bucket
							// transform into path through bundle
							bundles.push(edge_t{prev_source, e.u});
						} else { // same max, new bucket
							// last one from previous bucket goes inter-bucket
							bundles.push(edge_t{prev_source, prev_max});
						}
					} else { // new max found
						bundles.push(edge_t{prev_source, prev_max});
					}
					prev_source = e.u;
					prev_source_bundle = source_bundle;
					prev_max = e.v;
				}
				// one left over to push
				bundles.push(edge_t{prev_source, prev_max});
			} else {
				upper_part.rewind();
				for (; !upper_part.empty(); ++upper_part) {
					const edge_t& edge = *upper_part;
					const node_t comp = components.get(edge.u, edge.u);
					assert(edge.v <= maximas[comp]);
					const node_t max = maximas[comp];
					if (edge.v != max) {
						bundles.push(edge_t{edge.v, max});
					}
				}
			}
		} else { // at last bundle
			assert(bundles.get_interbundle_edges(bundle_id).size() == 0);
		}

	output_tree_edges:
		// --- generate tree edges
		for (node_t u=bundles.upper_boundary(bundle_id); u>=bundles.lower_boundary(bundle_id); --u) {
			if (TLX_UNLIKELY(!components.contains(u) && !maximas.contains(u))) {
				// there could be IDs never appearing in some graphs
				continue;
			}
			// note: will intentionally push self-loops for final component roots
			const node_t comp = components.get(u, u);
			maximas.insert_or_max(comp, u);
			const node_t max = maximas[comp];
			tree_pq.push(edge_t{max, u});
		}

	}
};

#endif //EM_CC_SIBEYNWITHBUNDLES_H
