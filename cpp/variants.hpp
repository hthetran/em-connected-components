#pragma once

#include <cmath>

struct policy_t {
	std::function<bool(size_t n, size_t m, unsigned level, size_t M)> perform_contraction;
	std::function<size_t(size_t n, size_t m, unsigned level, size_t M)> contract_number;
	std::function<int(size_t n, size_t m, unsigned level, size_t M)> sample_prob_power;
};

inline int nearest_power_reciprocal(size_t n, size_t m) {
	return std::max(1, static_cast<int>(std::floor(std::log2(static_cast<double>(m)/static_cast<double>(n)))));
}

inline size_t threshold_fun_4(size_t n, size_t M) {
	return std::min(4ul, static_cast<size_t>(2.0*(1.0+(2.0*static_cast<double>(M))/static_cast<double>(n))));
}

inline size_t threshold_fun_8(size_t n, size_t M) {
	return std::min(8ul, static_cast<size_t>(2.0*(1.0+(6.0*static_cast<double>(M))/static_cast<double>(n))));
}

policy_t variant_policies[] =
	{
		{ // 0: default; always contract, contract n/2 and sample with p=1/2 (for KKT in particular)
			[](size_t, size_t, unsigned, size_t) {return true;},
			[](size_t n, size_t, unsigned, size_t) {return n/2;},
			[](size_t, size_t, unsigned, size_t) {return 1;},
		},
		{ // 1+: setup for the experiments Rolf suggested
			[](size_t, size_t, unsigned level, size_t) {return level > 0;},
			[](size_t n, size_t, unsigned, size_t) {return n/2;},
			[](size_t, size_t, unsigned, size_t) {return 1;},
		},
		{ // 2
			[](size_t, size_t, unsigned level, size_t) {return level > 0;},
			[](size_t n, size_t, unsigned, size_t) {return n/2;},
			[](size_t, size_t, unsigned, size_t) {return 2;},
		},
		{ // 3
			[](size_t, size_t, unsigned level, size_t) {return level > 0;},
			[](size_t n, size_t, unsigned, size_t) {return n/2;},
			[](size_t, size_t, unsigned, size_t) {return 3;},
		},
		{ // 4
			[](size_t, size_t, unsigned level, size_t) {return level > 0;},
			[](size_t n, size_t, unsigned, size_t) {return n/2;},
			[](size_t, size_t, unsigned, size_t) {return 4;},
		},
		{ // 5
			[](size_t, size_t, unsigned level, size_t) {return level > 0;},
			[](size_t n, size_t, unsigned, size_t) {return n/2;},
			[](size_t, size_t, unsigned, size_t) {return 5;},
		},
		{ // 6+: adaptive (here: threshold 4)
			[](size_t n, size_t m, unsigned, size_t) {return (m/n)<4;},
			[](size_t n, size_t m, unsigned, size_t) {return n-m/4;},
			[](size_t n, size_t m, unsigned, size_t) {return nearest_power_reciprocal(n, m);},
		},
		{ // 7: threshold 8
			[](size_t n, size_t m, unsigned, size_t) {return (m/n)<8;},
			[](size_t n, size_t m, unsigned, size_t) {return n-m/8;},
			[](size_t n, size_t m, unsigned, size_t) {return nearest_power_reciprocal(n, m);},
		},
		{ // 8: threshold 4 for V/M=2, going to 2 as V/M increases
			[](size_t n, size_t m, unsigned, size_t M) {
				const auto threshold = threshold_fun_4(n, M);
				std::cout << "Contraction density threshold: " << threshold << std::endl;
				return (m/n) < threshold;
			},
			[](size_t n, size_t m, unsigned, size_t M) {
				const auto threshold = threshold_fun_4(n, M);
				return n - m/threshold;
			},
			[](size_t n, size_t m, unsigned, size_t) {return nearest_power_reciprocal(n, m);},
		},
		{ // 9: threshold 8 for V/M=2, going to 2 as V/M increases
			[](size_t n, size_t m, unsigned, size_t M) {
				const auto threshold = threshold_fun_8(n, M);
				std::cout << "Contraction density threshold: " << threshold << std::endl;
				return (m/n) < threshold;
			},
			[](size_t n, size_t m, unsigned, size_t M) {
				const auto threshold = threshold_fun_8(n, M);
				return n - m/threshold;
			},
			[](size_t n, size_t m, unsigned, size_t) {return nearest_power_reciprocal(n, m);},
		},
	};
