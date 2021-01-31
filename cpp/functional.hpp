/*
 * ConnectedComponents.cpp
 *
 * Created on: Feb 03, 2020
 *     Author: Hung Tran (hung@ae.cs.uni-frankfurt.de)
 */
#pragma once

#include <iostream>
#include <cassert>
#include <random>

#include <foxxll/common/timer.hpp>
#include "contraction.hpp"
#include "kruskal.hpp"
#include "sibeyn.hpp"
#include "util.hpp"
#include "vector-checks.hpp"
#include "variants.hpp"

class FunctionalConnectedComponents {
private:
	em_edge_vector& E;
	size_t node_bound;
	size_t semiext_bound;
	std::mt19937_64 &random_gen;
	unsigned recursion_level;
	policy_t& policy;

public:
	FunctionalConnectedComponents(em_edge_vector& input_edges, size_t n, size_t goal,
								  std::mt19937_64& gen, unsigned level, policy_t& variant) :
		E(input_edges),
		node_bound(n),
		semiext_bound(goal),
		random_gen(gen),
		recursion_level(level),
		policy(variant)
	{ }

	void run(em_mapping& components);

private:
	em_mapping _contract_nodes(size_t contract_num);
	void _sample_edges(em_edge_vector& notsampled_edges, em_edge_vector& sampled_edges, double p);
	void _reintegrate(em_mapping& node_components);
};


void FunctionalConnectedComponents::run(em_mapping& components) {
	std::cout << "call," << node_bound << "," << E.size() << "," << std::endl;
	foxxll::timer full_timer;
	foxxll::timer timer;
	full_timer.start();
	if (node_bound <= semiext_bound) {
		std::cout << "base_case,,," << std::endl;
		timer.start();
		SEMKruskal kruskal(E, 0, E.size(), components);
		timer.stop();
		std::cout << "kruskal,,," << timer.useconds() << std::endl;
		timer.reset();
		stxxl::sort(components.begin(), components.end(), edge_lt_ordering(), INTERNAL_SORT_MEM);
		full_timer.stop();
		std::cout << "done,,," << full_timer.useconds() << std::endl;
		return;
	}
	assert(is_sorted(E, edge_lt_ordering()));
	std::cout << "recursive_case,,," << std::endl;
	em_mapping contraction_stars;
	bool perform_contraction = policy.perform_contraction(node_bound, E.size(), recursion_level);
	if (perform_contraction) {
		// TODO: handle allocation better
		timer.start();
		size_t num_nodes_pre = node_bound;
		size_t contract_num = policy.contract_number(node_bound, E.size(), recursion_level);
		contraction_stars = _contract_nodes(contract_num);
		timer.stop();
		std::cout << "contraction," << num_nodes_pre << "," << node_bound << "," << timer.useconds() << std::endl;
		timer.reset();
		assert(is_sorted(contraction_stars, edge_lt_ordering()));
		assert(is_sorted(E, edge_lt_ordering()));
		assert(contains_stars_only(contraction_stars));
	}

	// 1. sample edges
	timer.start();
	em_edge_vector E1; // will contain E₁
	em_edge_vector E2; // edges initially not sampled
	double sample_prob = policy.sample_prob(node_bound, E.size(), recursion_level);
	_sample_edges(E1, E2, sample_prob);
	timer.stop();
	std::cout << "sample," << E1.size() << "," << E2.size() << "," << timer.useconds() << std::endl;
	timer.reset();

	// 2. compute connected components of the sampled edges
	em_mapping CC_G1; // will contain CC(G₁)
	timer.start();
	size_t num_nodes_in_E1 = number_of_nodes(E1); // TODO: cheaper count
	timer.stop();
	std::cout << "count,,," << timer.useconds() << std::endl;
	timer.reset();
	FunctionalConnectedComponents algo_sampled(E1, num_nodes_in_E1, semiext_bound, random_gen, recursion_level+1, policy);
	algo_sampled.run(CC_G1);
	std::cout << "cc_g1," << "," << CC_G1.size() << "," << std::endl;
	assert(contains_stars_only(CC_G1));

	// 3. now contract to get G' = G/CC(G₁)
	size_t node_bound_G_ = node_bound - CC_G1.size();
	bool right_goes_to_basecase = node_bound_G_ <= semiext_bound;
	if (right_goes_to_basecase) {
		timer.start();
		contract_stars_for_basecase(E2, CC_G1); // E2 now holds G'
		timer.stop();
		std::cout << "opt_g_,," << E2.size() << "," << timer.useconds() << std::endl;
		timer.reset();
	} else {
		timer.start();
		contract_stars(E2, CC_G1); // E2 now holds G'
		timer.stop();
		std::cout << "g_,," << E2.size() << "," << timer.useconds() << std::endl;
		timer.reset();
	}

	// 4.
	size_t num_nodes_in_G_ = node_bound_G_;
	if (!right_goes_to_basecase) {
		timer.start();
		num_nodes_in_G_ = number_of_nodes(E2); // TODO: drop this count
		assert(node_bound_G_ >= num_nodes_in_G_);
		timer.stop();
		std::cout << "count,,," << timer.useconds() << std::endl;
		timer.reset();
	}
	FunctionalConnectedComponents algo_second(E2, num_nodes_in_G_, semiext_bound, random_gen, recursion_level+1, policy);
	em_mapping CC_G_; // will hold CC(G')
	algo_second.run(CC_G_);
	std::cout << "cc_g_,," << CC_G_.size() << "," << std::endl;
	assert(contains_stars_only(CC_G_));

	// 5.
	timer.start();
	relabel(CC_G1, CC_G_); // RL(CC(G'), CC(G₁))
	timer.stop();
	std::cout << "relabel,,," << timer.useconds() << std::endl;
	timer.reset();

	// merging CC(G') and RL(CC(G'), CC(G₁))
	timer.start();
	components = CC_G_;
	for (const auto& e: CC_G1) {
		components.push_back(e);
	}
	// TODO: merge instead of concat + sort
	stxxl::sort(components.begin(), components.end(), edge_lt_ordering(), INTERNAL_SORT_MEM);
	assert(contains_stars_only(components));
	timer.stop();
	std::cout << "merge_original,,," << timer.useconds() << std::endl;
	timer.reset();

	if (perform_contraction) {
		assert(disjoint_sources(components, contraction_stars));
		// merging contraction stars with the above
		timer.start();
		relabel_targets(contraction_stars, components);
		for (const auto& e: contraction_stars) {
			components.push_back(e);
		}
		stxxl::sort(components.begin(), components.end(), edge_lt_ordering(), INTERNAL_SORT_MEM);
		assert(contains_stars_only(components));
		timer.stop();
		std::cout << "merge_contracted,,," << timer.useconds() << std::endl;
		timer.reset();
	}
	full_timer.stop();
	std::cout << "done,,," << full_timer.useconds() << std::endl;
}

em_mapping FunctionalConnectedComponents::_contract_nodes(size_t contract_num) {
	em_edge_vector tree;
	em_edge_vector leftover;
	run_sibeyn(contract_num, E, tree, leftover);
	orient_larger_to_smaller(tree);
	em_edge_vector stars;
	tfp(tree, stars);
	stxxl::sort(stars.begin(), stars.end(), edge_lt_ordering(), INTERNAL_SORT_MEM);
	stxxl::sort(leftover.begin(), leftover.end(), edge_lt_ordering(), INTERNAL_SORT_MEM);
	assert(disjoint_sources(stars, leftover));
	E = leftover;
	node_bound = node_bound - contract_num;
	return stars;
}

void FunctionalConnectedComponents::_sample_edges(em_edge_vector &sampled, em_edge_vector &notsampled, double p) {
	std::bernoulli_distribution coin(p);
	em_edge_vector::bufwriter_type sampled_w(sampled);
	em_edge_vector::bufwriter_type notsampled_w(notsampled);
	for (const auto& edge : E) {
		// with probability p the edge is sampled.
		if (coin(random_gen)) {
			sampled_w << edge;
		} else {
			notsampled_w << edge;
		}
	}
	sampled_w.finish();
	notsampled_w.finish();
	assert(sampled.size() + notsampled.size() == E.size());
}
