#pragma once

#include <cstdint>

class Game;

struct Player {
  Game* g;
  std::uint32_t id;
	int x;
	int y;
};
