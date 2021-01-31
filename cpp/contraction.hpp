#pragma once

#include <random>
#include <stxxl/sort>
#include "defs.hpp"
#include "vector-checks.hpp"

em_edge_vector* sample_out_edges(const em_edge_vector& E, std::mt19937_64 &gen) {
	em_edge_vector* sample = new em_edge_vector;
	// assumes edges already sorted by source
	// i: first index of edge with source u
	// j: first index of edge with next source
	unsigned long i=0;
	unsigned long j=0;
	while (i<E.size()) {
		const node_t& current_source = E[i].u;
		while (j<E.size() && E[j].u == current_source) {
			j++;
		}
		long u_degree = j-i;
		long offset = std::uniform_int_distribution<long>{0, u_degree-1}(gen);
		sample->push_back(E[i+offset]);
		i = j;
	}
	return sample;
}

void break_paths(em_edge_vector& E) {
	// assumes E is sorted by source
	em_node_vector right_endpoints;
	for (const edge_t& e: E) {
		right_endpoints.push_back(e.v);
	}
	stxxl::sort(right_endpoints.begin(), right_endpoints.end(),
	            node_lt_ordering(), INTERNAL_SORT_MEM);
	em_node_vector::const_iterator target_it = right_endpoints.begin();
	size_t j=0;
	for (edge_t& e: E) {
		while (target_it != right_endpoints.end() && *target_it < e.u) {
			target_it++;
		}
		if (*target_it != e.u) {
			E[j] = e;
			j++;
		}
	}
	E.resize(j);
}

void _relabel_sources(em_edge_vector& E, const em_edge_vector& stars) {
		em_edge_vector::bufreader_type star_reader(stars);
		em_edge_vector::bufwriter_type edge_writer(E);
		for (em_edge_vector::bufreader_type edge_reader(E); !edge_reader.empty(); ++edge_reader) {
			edge_t e = *edge_reader;
			while (!star_reader.empty() && (*star_reader).u < e.u) {
				++star_reader;
			}
			if (!star_reader.empty() && e.u == (*star_reader).u) {
				e.u = (*star_reader).v;
			}
			edge_writer << e;
		}
		edge_writer.finish();
}

void _relabel_targets(em_edge_vector& E, const em_edge_vector& stars) {
	em_edge_vector::bufreader_type star_reader(stars);
	em_edge_vector::bufwriter_type edge_writer(E);
	for (em_edge_vector::bufreader_type edge_reader(E); !edge_reader.empty(); ++edge_reader) {
		edge_t e = *edge_reader;
		while (!star_reader.empty() && (*star_reader).u < e.v) {
			++star_reader;
		}
		if (!star_reader.empty() && e.v == (*star_reader).u) {
			e.v = (*star_reader).v;
		}
		edge_writer << e;
	}
	edge_writer.finish();

}

void contract_stars(em_edge_vector& E, const em_edge_vector& stars) {
	// assumes: E and stars already sorted by lt_ordering
	_relabel_sources(E, stars);

	stxxl::sort(E.begin(), E.end(), edge_target_lt_ordering(), INTERNAL_SORT_MEM);
	{
		em_edge_vector::bufreader_type star_reader(stars);
		em_edge_vector::bufwriter_type edge_writer(E);
		for (em_edge_vector::bufreader_type edge_reader(E); !edge_reader.empty(); ++edge_reader) {
			edge_t e = *edge_reader;
			while (!star_reader.empty() && (*star_reader).u < e.v) {
				++star_reader;
			}
			if (!star_reader.empty() && e.v == (*star_reader).u) {
				e.v = (*star_reader).v;
			}
			e.orient_smaller_to_larger();
			edge_writer << e;
		}
		edge_writer.finish();
	}

	stxxl::sort(E.begin(), E.end(), edge_lt_ordering(), INTERNAL_SORT_MEM);
	{
		edge_t prev = MIN_EDGE;
		size_t edges_left = 0; // will remove self-loops, parallel edges
		em_edge_vector::bufwriter_type edge_writer(E);
		for (em_edge_vector::bufreader_type edge_reader(E); !edge_reader.empty(); ++edge_reader) {
			const edge_t& e = *edge_reader;
			if (!e.self_loop() && e != prev) {
				edge_writer << e;
				++edges_left;
				prev = e;
			}
		}
		edge_writer.finish();
		E.resize(edges_left);
	}
}

void contract_stars_for_basecase(em_edge_vector& E, const em_edge_vector& stars) {
	// assumes: E and stars already sorted by lt_ordering
	// but will *not* sort the contracted edges!
	// also, the removal of parallel edges will not be thorough
	_relabel_sources(E, stars);

	stxxl::sort(E.begin(), E.end(), edge_target_lt_ordering(), INTERNAL_SORT_MEM);
	{
		edge_t prev = MIN_EDGE;
		size_t edges_left = 0; // will remove self-loops, maybe some parallel edges
		em_edge_vector::bufreader_type star_reader(stars);
		em_edge_vector::bufwriter_type edge_writer(E);
		for (em_edge_vector::bufreader_type edge_reader(E); !edge_reader.empty(); ++edge_reader) {
			edge_t e = *edge_reader;
			while (!star_reader.empty() && (*star_reader).u < e.v) {
				++star_reader;
			}
			if (!star_reader.empty() && e.v == (*star_reader).u) {
				e.v = (*star_reader).v;
			}
			e.orient_smaller_to_larger();
			if (!e.self_loop() && e != prev) {
				edge_writer << e;
				++edges_left;
				prev = e;
			}
		}
		edge_writer.finish();
		E.resize(edges_left);
	}
}

void contract(em_edge_vector& E, std::mt19937_64 &gen) {
	em_edge_vector* sample = sample_out_edges(E, gen);
	break_paths(*sample);
	contract_stars(E, *sample);
}

void relabel(em_edge_vector& E, const em_edge_vector& stars) {
	// assumes: E and stars already sorted by lt_ordering
	_relabel_sources(E, stars);

	stxxl::sort(E.begin(), E.end(), edge_target_lt_ordering(), INTERNAL_SORT_MEM);
	_relabel_targets(E, stars);
	stxxl::sort(E.begin(), E.end(), edge_lt_ordering(), INTERNAL_SORT_MEM);
}

void relabel_targets(em_edge_vector& E, const em_edge_vector& stars) {
	assert(is_sorted(stars, edge_lt_ordering()));
	assert(contains_stars_only(stars));
	stxxl::sort(E.begin(), E.end(), edge_target_lt_ordering(), INTERNAL_SORT_MEM);
	_relabel_targets(E, stars);
	stxxl::sort(E.begin(), E.end(), edge_lt_ordering(), INTERNAL_SORT_MEM);
}
