#pragma once

#include "TopicTree.h"
#include <cstdint>
#include <string_view>

#include <App.h>

#include <utils/SeededMt19937.hpp>

class Player;

class Game {
public:
	static constexpr std::uint32_t PLAYERS_PER_ROUND = 10;
	static constexpr int MAP_WIDTH = 30;
	static constexpr int MAP_HEIGHT = 20;

private:
  static inline SeededMt19937 rng;

	std::uint32_t gameId;
	uWS::App& app;
	std::array<char, MAP_WIDTH * MAP_HEIGHT> map;
	
public:
	Game(uWS::App&, std::uint32_t id);
	
	std::string_view getTopicName() const;
	uWS::Topic * getTopic() const;

	bool isIdle() const;
	std::uint32_t getNumPlayers() const;

  void newPlayer(Player&, uWS::WebSocket<false, true, Player>*);
  void movePlayer(Player&, uWS::WebSocket<false, true, Player>*, char key);
  void playerLeft(Player&, uWS::WebSocket<false, true, Player>*);

private:
  void broadcastGameInfo() const;
  void broadcastPlayerData() const;
  void sendMap(uWS::WebSocket<false, true, Player>*) const;

	void initMap();
  char getCell(int x, int y) const;
};
