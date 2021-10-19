#include "Game.hpp"
#include <algorithm>
#include <cstring>
#include <charconv>
#include <exception>
#include <string_view>
#include <system_error>

Game::Game(uWS::App& app, std::uint32_t id)
: gameId(id),
  app(app) { }

std::string_view Game::getTopic() const {
  static char gameTopic[12] = "g/";

	if (auto [ptr, ec] = std::to_chars(std::begin(gameTopic) + 2, std::end(gameTopic), gameId);
			ec == std::errc()) {
		return {gameTopic, static_cast<std::size_t>(std::distance(gameTopic, ptr))};
	}
	
  return {};
}

bool Game::isIdle() const {
  return false;
}

std::uint32_t Game::getNumPlayers() const {
  uWS::Topic * t = app.topicTree ? app.topicTree->lookupTopic(getTopic()) : nullptr;
  return t ? t->size() : 0;
}

void Game::initMap() {
	static const char keys[] = {
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
		'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
		'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.'
	};

	map.fill('\0');

	for (std::size_t y = 0; y < MAP_HEIGHT; y++) {
		for (std::size_t x = 0; x < MAP_WIDTH; x++) {
			
		}
	}
}