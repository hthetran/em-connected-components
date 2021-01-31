/*
 * PowerOfTwoCoin.h
 *
 * Copyright (C) 2020 David Hammer <hammer@ae.cs.uni-frankfurt.de>
 */

#pragma once

// will return true with probability 1/2^power
class PowerOfTwoCoin {
public:
	explicit PowerOfTwoCoin(int power_) : power(power_),
										  bits_left(0),
										  random_bits(0) {
		// note: assumes power is not 64
		mask = ~(~0lu << power);
	}

	template <typename Gen>
	bool operator() (Gen& gen) {
		if (bits_left < power) {
			random_bits = dist(gen);
			bits_left = 64;
		}
		bool res = (random_bits & mask) == mask;
		random_bits = random_bits >> power;
		bits_left -= power;
		return res;
	}

	[[nodiscard]] double get_probability() const {
		return 1/static_cast<double>(1 << power);
	}

	void reset() {
		dist.reset();
		bits_left = 0;
		random_bits = 0;
	}

private:
	std::uniform_int_distribution<uint64_t> dist;
	int power;
	int bits_left;
	uint64_t random_bits;
	uint64_t mask;
};
