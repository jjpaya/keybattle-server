#pragma once

#include <cstdint>
#include <string_view>

#include <App.h>

class Game {
public:
	static constexpr std::uint32_t PLAYERS_PER_ROUND = 10;
	static constexpr std::uint32_t MAP_WIDTH = 30;
	static constexpr std::uint32_t MAP_HEIGHT = 20;

private:
	std::uint32_t gameId;
	uWS::App& app;
	std::array<char, MAP_WIDTH * MAP_HEIGHT> map;
	
public:
	Game(uWS::App&, std::uint32_t id);
	
	std::string_view getTopic() const;

	bool isIdle() const;
	std::uint32_t getNumPlayers() const;

private:
	void initMap();
};
