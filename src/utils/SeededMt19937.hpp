#pragma once

#include <random>

struct SeededMt19937 : std::mt19937 {
public:
	SeededMt19937();
};
