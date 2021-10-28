#include "SeededMt19937.hpp"
#include "RandomSeeder.hpp"

SeededMt19937::SeededMt19937() {
	// RandomSeeder fills the internal state of the mersenne twister with random
	// numbers from std::random_device
	RandomSeeder rs{};
	std::mt19937::seed(rs);
}
