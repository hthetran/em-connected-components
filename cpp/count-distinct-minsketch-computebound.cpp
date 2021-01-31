/*
 * count-distinct-minsketch-computebound.cpp
 *
 * Copyright (C) 2020 Hung Tran <hung@ae.cs.uni-frankfurt.de>
 */

#include <random>
#include <chrono>
#include <stxxl/sorter>
#include "streaming/utils/ForLoopSingleArg.h"
#include "streaming/containers/EdgeStream.h"
#include "streaming/distinct_elements/ApplyMeans.h"
#include "streaming/distinct_elements/ApplyMedians.h"
#include "streaming/distinct_elements/MinSketch.h"
#include "streaming/utils/StreamPusher.h"
#include "streaming/utils/PowerOfTwoCoin.h"
#include "streaming/distinct_elements/multiply_shift_hash.h"

static const size_t node_size = 1u << 20u;
static const size_t graph_size = 1e8;
static const size_t repetitions = 9;

struct TryVariant {
    template <typename Edges, typename Gen, typename Estimator>
    void operator()(Edges& es, Gen& gen, size_t power, const std::string& label, Estimator& estimator){
        size_t cum_counts = 0;
        size_t cum_elapsed_time = 0;
        for (size_t i = 0; i < repetitions; ++i) {
            std::cout << power << "," << i << "," << label << ",";

            EdgeStream sample_left;
            EdgeStream sample_right;
            PowerOfTwoCoin coin(power);

            auto timer_begin = std::chrono::high_resolution_clock::now();
            es.rewind();
            for (; !es.empty(); ++es) {
                const auto o = *es;
                if (coin(gen)) {
                    estimator(o.u);
                    estimator(o.v);
                    sample_left.push(o);
                } else
                    sample_right.push(o);
            }
            auto timer_end = std::chrono::high_resolution_clock::now();
            auto timer_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(timer_end - timer_begin).count();
            std::cout << timer_elapsed << std::endl;

            cum_counts += estimator.count();
            cum_elapsed_time += timer_elapsed;

            size_t doo_sum_left = 0;
            size_t doo_sum_right = 0;
            sample_left.rewind();
            for (; !sample_left.empty(); ++sample_left) {
                doo_sum_left += (*sample_left).u;
                doo_sum_left -= (*sample_left).v;
            }
            sample_right.rewind();
            for (; !sample_right.empty(); ++sample_right) {
                doo_sum_right += (*sample_right).u;
                doo_sum_right -= (*sample_right).v;
            }
            std::cout << "doo" << doo_sum_left << doo_sum_right << std::endl;
        }
    }
};

int main() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<node_t> dis(1, node_size);

    // generate
    std::cout << "Generating input..." << std::endl;
    stxxl::sorter<edge_t, edge_less_cmp> es_sorter(edge_less_cmp(), SORTER_MEM);
    EdgeStream edges;
    for (size_t i = 0; i < graph_size; ++i) {
        es_sorter.push(edge_t{dis(gen), dis(gen)});
    }
    std::cout << "Generating input... done" << std::endl;

    // sort
    std::cout << "Sorting input..." << std::endl;
    es_sorter.sort_reuse();
    std::cout << "Sorting input... done" << std::endl;

    // push to stream
    std::cout << "Pushing input..." << std::endl;
    StreamPusher(es_sorter, edges);
    std::cout << "Pushing input... done" << std::endl;

    // probabilities
    for (size_t i = 1; i < 5; ++i) {
        TryVariant loop;

        // hash functions + repetitions
        ApplyMedians<5, ApplyMeans<5, MinSketch>> old_estimator(gen);
        loop(edges, gen, i, "old", old_estimator);

        ApplyMedians<5, ApplyMeans<5, NewMinSketch<MultiplyShiftHash32Bit, node_t>>> new_estimator32(gen, node_size);
        loop(edges, gen, i, "new32bit", new_estimator32);

        ApplyMedians<5, ApplyMeans<5, NewMinSketch<MultiplyShiftHash32Bit, node_t>>> new_estimator64(gen, node_size);
        loop(edges, gen, i, "new64bit", new_estimator64);

        // scanning time
        PowerOfTwoCoin coin(i);
        for (size_t j = 0; j < repetitions; ++j) {
            std::cout << i << ",0," << j << ",scan,";

            node_t min = MAX_NODE;
            EdgeStream sample_left;
            EdgeStream sample_right;

            auto scanning_timer_begin = std::chrono::high_resolution_clock::now();
            edges.rewind();
            for (; !edges.empty(); ++edges) {
                const auto edge = *edges;
                min = std::min(edge.u, min);
                min = std::min(edge.v, min);

                if (coin(gen))
                    sample_left.push(edge);
                else
                    sample_right.push(edge);
            }
            auto scanning_timer_end = std::chrono::high_resolution_clock::now();
            auto scanning_timer_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(scanning_timer_end - scanning_timer_begin).count();
            std::cout << scanning_timer_elapsed << std::endl;

            std::cout << "doo" << min << std::endl;
            std::cout << "doo" << sample_left.size() << std::endl;
            std::cout << "doo" << (*sample_left).u << std::endl;
            std::cout << "doo" << sample_right.size() << std::endl;
            std::cout << "doo" << (*sample_right).u << std::endl;
        }
    }

    return 0;
}
