#pragma once

#include <random>

#include <stxxl/sorter>
#include "defs.hpp"
#include "contraction.hpp"
#include "kruskal.hpp"
#include "stream-checks.hpp"
#include "stream-sibeyn.hpp"
#include "stream-utils.hpp"
#include "streaming/utils/StreamFilter.h"
#include "variants.hpp"



template <typename edge_stream_t>
void compute_ccs(edge_stream_t& input_stream, size_t node_estimate, size_t semiexternal_limit, edge_stream_t& output,
                 unsigned level, policy_t& policy) {
	foxxll::timer full_timer;
	foxxll::timer timer;
	std::cout << "call," << node_estimate << "," << input_stream.size() << "," << std::endl;
	full_timer.start();
	timer.start();
	size_t old_estimate = node_estimate;
	node_estimate = count_unique_nodes(input_stream);
	input_stream.rewind();
	timer.stop();
	std::cout << "count," << old_estimate << "," << node_estimate << "," << timer.useconds() << std::endl;
	timer.reset();

	// base case
	if (node_estimate <= semiexternal_limit) {
		// TODO: streamify instead of materializing vector
		std::cout << "base_case,,," << std::endl;
		timer.start();
		em_edge_vector edges(input_stream.size());
		stxxl::stream::materialize(input_stream, edges.begin(), edges.end());
		em_edge_vector stars;
		SEMKruskal kruskal_algo(edges, 0, edges.size(), stars);
		stxxl::sort(stars.begin(), stars.end(), edge_lt_ordering(), SORTER_MEM);
		for (const auto& e: stars) {
			output.push(e);
		}
		timer.stop();
		std::cout << "kruskal,,," << timer.useconds() << std::endl;
		full_timer.stop();
		std::cout << "done,,," << full_timer.useconds() << std::endl;
		return;
	}

	std::cout << "recursive_case,,," << std::endl;
	assert(is_sorted(input_stream, edge_lt_ordering()));

	bool perform_contraction = policy.perform_contraction(node_estimate, input_stream.size(), level);
	edge_stream_t contracted_edges;
	edge_stream_t contraction_stars;
	if (perform_contraction) {
		timer.start();
		// contraction
		size_t number_to_contract = policy.contract_number(node_estimate, input_stream.size(), level);
		old_estimate = node_estimate;
		contract_nodes(input_stream, number_to_contract, contracted_edges, contraction_stars);
		contracted_edges.consume();
		contraction_stars.consume();
		node_estimate -= number_to_contract;
		timer.stop();
		std::cout << "contraction," << old_estimate << "," << node_estimate << "," << timer.useconds() << std::endl;
		timer.reset();

		assert(disjoint_sources(contraction_stars, contracted_edges));
		assert(only_stars(contraction_stars));
		assert(is_sorted(contracted_edges, edge_lt_ordering()));
		assert(is_sorted(contraction_stars, edge_lt_ordering()));
	} else {
		// TODO: figure out something smarter
		flush(input_stream, contracted_edges);
		contracted_edges.consume();
	}


	// 1. sample edges
	timer.start();
	edge_stream_t E1;
	edge_stream_t E2;
	// TODO: parameter tweaking of 0.5
	double sample_prob = policy.sample_prob(node_estimate, contracted_edges.size(), level);
	sample_edges(contracted_edges, E1, E2, sample_prob);
	E1.consume();
	E2.consume();
	timer.stop();
	std::cout << "sample," << E1.size() << "," << E2.size() << "," << timer.useconds() << std::endl;
	timer.reset();
	assert(E1.size() + E2.size() == contracted_edges.size());

	// 2. compute CCs of sampled E1
	edge_stream_t CC_G1;
	// TODO: update node estimate more smartly
	compute_ccs(E1, node_estimate/2, semiexternal_limit, CC_G1, level+1, policy);
	CC_G1.consume();
	std::cout << "cc_g1,," << CC_G1.size() << "," << std::endl;
	assert(is_sorted(CC_G1, edge_lt_ordering()));
	assert(only_stars(CC_G1));

	// 3. contract to get G' = G/CC(G₁)
	timer.start();
	edge_stream_t G_;
	contract_stars(E2, CC_G1, G_);
	G_.consume();
	CC_G1.rewind();
	timer.stop();
	std::cout << "g_,," << G_.size() << "," << timer.useconds() << std::endl;
	assert(disjoint_sources(G_, CC_G1));
	assert(only_stars(CC_G1));

	// 4. recurse on G' to get CC(G')
	edge_stream_t CC_G_;
	// TODO: update node estimate smartly
	compute_ccs(G_, node_estimate/2, semiexternal_limit, CC_G_, level+1, policy);
	CC_G_.consume();
	std::cout << "cc_g_,," << CC_G_.size() << "," << std::endl;

	// 5. relabel CC(G₁) by CC(G'), merge and output
	timer.start();
	edge_stream_t relabeled_G1;
	relabel(CC_G1, CC_G_, relabeled_G1);
	relabeled_G1.consume();
	CC_G_.rewind();
	timer.stop();
	std::cout << "relabel,,," << timer.useconds() << std::endl;
	timer.reset();
	assert(only_stars(relabeled_G1));

	timer.start();
	StreamMerger functional_output(relabeled_G1, CC_G_, edge_lt_ordering());;
	assert(only_stars(functional_output));
	assert(relabeled_G1.size() + CC_G_.size() == functional_output.size());

	if (perform_contraction) {
		// update the targets of set aside contraction stars
		edge_stream_t updated_contraction_stars;
		relabel_targets(contraction_stars, functional_output, updated_contraction_stars);
		updated_contraction_stars.consume();
		functional_output.rewind();

		// merge in the contracted stars
		assert(disjoint_sources(contraction_stars, functional_output));
		StreamMerger merged_stars(functional_output, updated_contraction_stars, edge_lt_ordering());
		flush(merged_stars, output);
		assert(functional_output.size() + contraction_stars.size() == output.size());
	} else {
		// TODO: smarter
		flush(functional_output, output);
	}
	timer.stop();
	std::cout << "merge,,," << timer.useconds() << std::endl;
	full_timer.stop();
	std::cout << "done,,," << full_timer.useconds() << std::endl;
}

template <typename edge_stream_t>
size_t count_unique_nodes(edge_stream_t& edges) {
	// TODO: check if it's faster to split into two node lists and then merge
	using node_sorter = stxxl::sorter<node_t, node_lt_ordering>;
	node_sorter node_set(node_lt_ordering(), SORTER_MEM);
	node_t prev_source = MIN_NODE;
	for (; !edges.empty(); ++edges) {
		if ((*edges).u != prev_source) {
			node_set.push((*edges).u);
			prev_source = (*edges).u;
		}
		node_set.push((*edges).v);
	}
	node_set.sort();
	UniqueElementStreamFilter<node_sorter> unique_nodes(node_set);
	size_t count = 0;
	for (; !unique_nodes.empty(); ++unique_nodes) {
		++count;
	}
	return count;
}

// turn this function into a stream type instead?
template <typename edge_stream_t>
void contract_stars(edge_stream_t& edges, edge_stream_t& stars, edge_stream_t& output) {
	// assumes edges and stars are already sorted
	// and that we can rewind stars
	// TODO: think about this more (buffering between levels?)
	assert(is_sorted(edges, edge_lt_ordering()));
	assert(is_sorted(stars, edge_lt_ordering()));
	assert(only_stars(stars));
	using edge_target_sorter = stxxl::sorter<edge_t, edge_target_lt_ordering>;
	edge_target_sorter updated_sources(edge_target_lt_ordering(), SORTER_MEM);
	// note: needs to be strong cmp for unique filtering after
	for (; !edges.empty(); ++edges) {
		const edge_t e = *edges;
		while (!stars.empty() && (*stars).u < e.u) {
			++stars;
		}
		if (!stars.empty() && (*stars).u == e.u) {
			updated_sources.push(edge_t((*stars).v, e.v));
		} else {
			updated_sources.push(e);
		}
	}
	updated_sources.sort();

	// make unique
	UniqueElementStreamFilter<edge_target_sorter> updated_sources_unique(updated_sources);
	// update targets
	stars.rewind();
	using edge_source_sorter = stxxl::sorter<edge_t, edge_lt_ordering>;
	edge_source_sorter updated_targets(edge_lt_ordering(), SORTER_MEM);
	for (; !updated_sources_unique.empty(); ++updated_sources_unique) {
		const edge_t e = *updated_sources_unique;
		while (!stars.empty() && (*stars).u < e.v) {
			++stars;
		}
		// making sure that resulting edges are still smaller-to-larger
		edge_t new_edge = e;
		if (!stars.empty() && (*stars).u == e.v) {
			new_edge = edge_t(e.u, (*stars).v);
		}
		new_edge.orient_smaller_to_larger();
		if (!new_edge.self_loop()) {
			updated_targets.push(new_edge);
		}
	}
	updated_targets.sort();

	// make unique and output
	UniqueElementStreamFilter<edge_source_sorter> updated_targets_unique(updated_targets);
	flush(updated_targets_unique, output);
}

// turn this function into a stream type instead?
template <typename edge_stream_t>
void relabel(edge_stream_t& edges, edge_stream_t& stars, edge_stream_t& output) {
	// assumes edges and stars are already sorted
	// and that we can rewind stars
	// TODO: think about this more (buffering between levels?)
	assert(is_sorted(edges, edge_lt_ordering()));
	assert(is_sorted(stars, edge_lt_ordering()));
	assert(only_stars(stars));
	using edge_target_sorter = stxxl::sorter<edge_t, edge_target_lt_ordering>;
	edge_target_sorter updated_sources(edge_target_lt_ordering(), SORTER_MEM);
	// note: needs to be strong cmp for unique filtering after
	for (; !edges.empty(); ++edges) {
		const edge_t e = *edges;
		while (!stars.empty() && (*stars).u < e.u) {
			++stars;
		}
		if (!stars.empty() && (*stars).u == e.u) {
			updated_sources.push(edge_t((*stars).v, e.v));
		} else {
			updated_sources.push(e);
		}
	}
	updated_sources.sort();

	// make unique
	UniqueElementStreamFilter<edge_target_sorter> updated_sources_unique(updated_sources);
	// update targets
	stars.rewind();
	using edge_source_sorter = stxxl::sorter<edge_t, edge_lt_ordering>;
	edge_source_sorter updated_targets(edge_lt_ordering(), SORTER_MEM);
	for (; !updated_sources_unique.empty(); ++updated_sources_unique) {
		const edge_t e = *updated_sources_unique;
		while (!stars.empty() && (*stars).u < e.v) {
			++stars;
		}
		// making sure that resulting edges are still smaller-to-larger
		edge_t new_edge = e;
		if (!stars.empty() && (*stars).u == e.v) {
			new_edge = edge_t(e.u, (*stars).v);
		}
		// difference from contract: allow self-loop and do not reorient
		updated_targets.push(new_edge);
	}
	updated_targets.sort();

	// make unique and output
	UniqueElementStreamFilter<edge_source_sorter> updated_targets_unique(updated_targets);
	flush(updated_targets_unique, output);
}

template <typename edge_stream_1_t, typename edge_stream_2_t, typename edge_stream_3_t>
void relabel_targets(edge_stream_1_t& edges, edge_stream_2_t& stars, edge_stream_3_t& output) {
	// TODO: put the stars into a sorter earlier instead of flushing
	using edge_target_sorter = stxxl::sorter<edge_t, edge_target_lt_ordering>;
	edge_target_sorter sort_by_target(edge_target_lt_ordering(), SORTER_MEM);
	flush(edges, sort_by_target);
	sort_by_target.sort();

	using edge_source_sorter = stxxl::sorter<edge_t, edge_lt_ordering>;
	edge_source_sorter updated_targets(edge_lt_ordering(), SORTER_MEM);
	for (; !sort_by_target.empty(); ++sort_by_target) {
		const edge_t e = *sort_by_target;
		while (!stars.empty() && (*stars).u < e.v) {
			++stars;
		}
		if (!stars.empty() && (*stars).u == e.v) {
			updated_targets.push(edge_t(e.u, (*stars).v));
		} else {
			updated_targets.push(e);
		}
	}
	updated_targets.sort();
	flush(updated_targets, output);
}

template <typename edge_stream_t>
void sample_edges(edge_stream_t& in_stream, edge_stream_t& out_stream1, edge_stream_t& out_stream2, double sample_prob) {
	// TODO: maybe carry generator around
	std::default_random_engine gen;
	std::bernoulli_distribution coin(sample_prob);

	// push edges into respective sub-problem
	for (; !in_stream.empty(); ++in_stream) {
		if (coin(gen)) {
			out_stream1.push(*in_stream);
		} else {
			out_stream2.push(*in_stream);
		}
	}
}

template <typename edge_stream_t>
void contract_nodes(edge_stream_t& edges, size_t contract_num, edge_stream_t& output_edges, edge_stream_t& output_stars) {
	// TODO: make this streaming-based, too
	edge_stream_t tree;
	using edge_source_sorter = stxxl::sorter<edge_t, edge_lt_ordering>;
	edge_source_sorter sort_contracted_edges(edge_lt_ordering(), SORTER_MEM);
	run_sibeyn(edges, contract_num, tree, sort_contracted_edges);
	tree.consume();
	sort_contracted_edges.sort();
	flush(sort_contracted_edges, output_edges);
	StreamEdgesOrientReverse reversing_edges(tree);
	tfp(reversing_edges, output_stars);
}
