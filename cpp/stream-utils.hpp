#pragma once

#include "defs.hpp"


template <typename stream_1_t, typename stream_2_t, class comparator_t>
class StreamMerger {
public:
	using value_type = typename stream_1_t::value_type;
private:
	stream_1_t& stream_1;
	stream_2_t& stream_2;
	comparator_t cmp;
	value_type current;
	bool current_from_left;
	void update() {
		if (stream_1.empty()) {
			current = *stream_2;
			current_from_left = false;
		} else if (stream_2.empty()) {
			current = *stream_1;
			current_from_left = true;
		} else {
			if (cmp(*stream_1, *stream_2)) {
				current = *stream_1;
				current_from_left = true;
			} else {
				current = *stream_2;
				current_from_left = false;
			}
		}

	}
public:
	StreamMerger(stream_1_t& stream_1_, stream_2_t& stream_2_, comparator_t cmp_) :
		stream_1(stream_1_), stream_2(stream_2_), cmp(cmp_) {
		if (!empty()) {
			update();
		}
	}
	bool empty() const {
		return stream_1.empty() && stream_2.empty();
	}
	const value_type& operator* () const {
		return current;
	}
	StreamMerger& operator++ () {
		if (current_from_left) {
			++stream_1;
		} else {
			++stream_2;
		}
		if (!empty()) {
			update();
		}
		return *this;
	}
	size_t size() const {
		return stream_1.size() + stream_2.size();
	}
	void rewind() {
		stream_1.rewind();
		stream_2.rewind();
		if (!empty()) {
			update();
		}
	}
};

template <typename stream_1_t, typename stream_2_t, class comparator_t>
class StreamMergerUnique {
public:
	using value_type = typename stream_1_t::value_type;
private:
	stream_1_t& stream_1;
	stream_2_t& stream_2;
	comparator_t cmp;
	value_type current{MAX_NODE, MAX_NODE};
	bool current_from_left = true;
	void update() {
		while (!stream_1.empty() && !stream_2.empty()) {
			if (*stream_1 == *stream_2) {
				++stream_1;
			} else if (cmp(*stream_1, *stream_2)) {
				current = *stream_1;
				current_from_left = true;
				return;
			} else {
				current = *stream_2;
				current_from_left = false;
				return;
			}
		}
		if (stream_1.empty()) {
			current = *stream_2;
			current_from_left = false;
		} else if (stream_2.empty()) {
			current = *stream_1;
			current_from_left = true;
		}
	}
public:
	StreamMergerUnique(stream_1_t& stream_1_, stream_2_t& stream_2_, comparator_t cmp_) :
		stream_1(stream_1_), stream_2(stream_2_), cmp(cmp_) {
		if (!empty()) {
			update();
		}
	}
	bool empty() const {
		return stream_1.empty() && stream_2.empty();
	}
	const value_type& operator* () const {
		return current;
	}
	StreamMergerUnique& operator++ () {
		if (current_from_left) {
			++stream_1;
		} else {
			++stream_2;
		}
		if (!empty()) {
			update();
		}
		return *this;
	}
	size_t size() const {
		return stream_1.size() + stream_2.size();
	}
	void rewind() {
		stream_1.rewind();
		stream_2.rewind();
		if (!empty()) {
			update();
		}
	}
};


template <typename stream1_t, typename stream2_t>
void flush(stream1_t& input, stream2_t& output) {
	for (; !input.empty(); ++input) {
		output.push(*input);
	}
}

template <typename edge_stream_t>
class StreamEdgesOrientReverse {
public:
	using value_type = typename edge_stream_t::value_type;
private:
	edge_stream_t& input_stream;
	value_type current;
public:
	StreamEdgesOrientReverse(edge_stream_t& in): input_stream(in) {
		if (!input_stream.empty()) {
			if ((*input_stream).u < (*input_stream).v) {
				current = edge_t((*input_stream).v, (*input_stream).u);
			} else {
				current = *input_stream;
			}
		}
	}

	const value_type& operator* () const {
		return current;
	}

	StreamEdgesOrientReverse& operator++ () {
		++input_stream;
		if (!input_stream.empty()) {
			if ((*input_stream).u < (*input_stream).v) {
				current = edge_t((*input_stream).v, (*input_stream).u);
			} else {
				current = *input_stream;
			}
		}
		return *this;
	}

	bool empty() const {
		return input_stream.empty();
	}

	void rewind() {
		input_stream.rewind();
		if (!input_stream.empty()) {
			if ((*input_stream).u < (*input_stream).v) {
				current = edge_t((*input_stream).v, (*input_stream).u);
			} else {
				current = *input_stream;
			}
		}
	}
};

template <typename edge_stream_t>
class StreamEdgesOrientNormal {
public:
	using value_type = typename edge_stream_t::value_type;
private:
	edge_stream_t& input_stream;
	value_type current;
	void update() {
		if ((*input_stream).u > (*input_stream).v) {
			current = edge_t((*input_stream).v, (*input_stream).u);
		} else {
			current = *input_stream;
		}
	}
public:
	StreamEdgesOrientNormal(edge_stream_t& in): input_stream(in) {
		if (!empty()) {
			update();
		}
	}

	const value_type& operator* () const {
		return current;
	}

	StreamEdgesOrientNormal& operator++ () {
		++input_stream;
		if (!empty()) {
			update();
		}
		return *this;
	}

	bool empty() const {
		return input_stream.empty();
	}

	void rewind() {
		input_stream.rewind();
		if (!empty()) {
			update();
		}
	}
};
