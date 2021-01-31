#pragma once

template <typename key_type, typename value_type>
class SimpleShiftMap {

protected:
	key_type min_key;
	key_type max_key;
	size_t num_keys; // actual number of elements stored
	std::vector<value_type> data;
	std::vector<bool> filled;

public:
	// key range [min_key, max_key] inclusive
	SimpleShiftMap(key_type min_key, key_type max_key):
		min_key(min_key),
		max_key(max_key),
		num_keys(0),
		data((max_key - min_key)+1),
		filled((max_key - min_key)+1, false)
	{}

	bool valid_key(const key_type& k) {
		return min_key <= k && k <= max_key;
	}

	bool contains(const key_type& k) {
		if (!valid_key(k)) {
			return false;
		}
		const size_t index = key_to_index(k);
		return filled[index];
	}

	value_type get(const key_type& k, const value_type fallback) {
		if (contains(k)) {
			return at(k);
		} else {
			return fallback;
		}
	}

	void insert(const key_type& k, const value_type& v) {
		assert(valid_key(k));
		const size_t index = key_to_index(k);
		data[index] = v;
		if (!filled[index]) {
			++num_keys;
			filled[index] = true;
		}
	}

	void insert_or_max(const key_type& k, const value_type& v) {
		assert(valid_key(k));
		const size_t index = key_to_index(k);
		if (!filled[index]) {
			++num_keys;
			filled[index] = true;
			data[index] = v;
		} else {
			data[index] = std::max(data[index], v);
		}
	}

	// TODO: consider checking if set
	value_type& operator[](const key_type& k) {
		assert(valid_key(k));
		const size_t index = key_to_index(k);
		if (!filled[index]) {
			++num_keys;
			data[index] = value_type();
			filled[index] = true;
		}
		return data[index];
	}

	size_t size() {
		return num_keys;
	}

	template <typename receiving_stream_t>
	void push_mapping(receiving_stream_t& output) {
		key_type k = min_key;
		for (const value_type& v: data) {
			output.push({k, v});
			++k;
		}
	}

protected:
	value_type& at(const key_type& k) {
		return data[key_to_index(k)];
	}

	inline size_t key_to_index(const key_type& k) {
		return k - static_cast<size_t>(min_key);
	}

	inline key_type index_to_key(const size_t& i) {
		return static_cast<key_type>(i) + min_key;
	}
};
