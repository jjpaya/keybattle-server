#pragma once

#include <cstdint>
#include <chrono>

class Game;

struct Player {
  Game* g;
  std::uint32_t id;
	int x;
	int y;
  int points;
  std::chrono::system_clock::time_point frozenUntil;
};
