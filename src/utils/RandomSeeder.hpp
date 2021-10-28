#pragma once

#include <random>

class RandomSeeder {
	std::random_device rd;

public:
	RandomSeeder();

	using result_type = std::random_device::result_type;

	template<typename RandomAccessIterator>
	void generate(RandomAccessIterator, RandomAccessIterator);
};

#include "RandomSeeder.tpp"
