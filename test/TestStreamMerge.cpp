/*
 * TestStreamMerge.cpp
 *
 * Created on: Apr 16, 2019
 *     Author: Hung Tran (hung@ae.cs.uni-frankfurt.de)
 */


#include <gtest/gtest.h>
#include <stxxl/sequence>
#include "../cpp/streaming/utils/StreamMerge.h"

class TestStreamMerge : public ::testing::Test { };

TEST_F(TestStreamMerge, test_stxxl_sequence_int) {
    stxxl::sequence<int> a;
    stxxl::sequence<int> b;

    std::vector<int> a_vals = {3, 4, 6, 8, 9};
    std::vector<int> b_vals = {0, 1, 2, 5, 7};
    for (const auto val : a_vals)
        a.push_back(val);
    for (const auto val : b_vals)
        b.push_back(val);

    struct SelectSmaller {
        bool select(const int & a, const int & b) const {
            return a > b;
        }
    };

    struct Extractor {
        using value_type = int;
        value_type extract(const int & a, const bool & flag) const {
            tlx::unused(flag);
            return a;
        }
    };

    auto as = a.get_stream();
    auto bs = b.get_stream();
    const SelectSmaller s;
    const Extractor e;

    TwoWayStreamMerge<decltype(as), decltype(bs), SelectSmaller, Extractor> merge(as, bs, s, e);
    std::vector<int> output;
    for (; !merge.empty(); ++merge)
        output.push_back(*merge);

    std::vector<int> expected_output = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    ASSERT_EQ(output, expected_output);
}