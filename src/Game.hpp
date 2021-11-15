#pragma once

#include "TopicTree.h"
#include <cstdint>
#include <string_view>
#include <chrono>
#include <vector>

#include <App.h>

#include <utils/SeededMt19937.hpp>
#include <utils/IdSys.hpp>

class Player;

class Game {
public:
	static constexpr std::uint32_t PLAYERS_PER_ROUND = 10;
	static constexpr int MAP_WIDTH = 30;
	static constexpr int MAP_HEIGHT = 20;
  enum class State : std::uint16_t {
    IDLE,
    PLAYING,
    ENDED
  };

  struct Point {
    int x;
    int y;
  };

  struct PaintEvent {
    int x;
    int y;
    std::uint8_t color;
  };

private:
  static inline SeededMt19937 rng;

	std::uint32_t gameId;
	uWS::App& app;

  IdSys<std::uint32_t> playerIds;

	std::array<char, MAP_WIDTH * MAP_HEIGHT> map;
	std::array<std::uint8_t, MAP_WIDTH * MAP_HEIGHT> paintMap;
  std::array<std::uint32_t, PLAYERS_PER_ROUND> playerColors;

  std::vector<PaintEvent> paintUpdates;

  State state;
  std::chrono::system_clock::time_point stateTime;

  bool sendPlayerDataOnTick;
	
public:
	Game(uWS::App&, std::uint32_t id);
	
	std::string_view getTopicName() const;
	uWS::Topic * getTopic() const;

	void setState(State);
	State getState() const;
	std::uint32_t getNumPlayers() const;
  Player getNewPlayer();

  void newPlayer(Player&, uWS::WebSocket<false, true, Player>*);
  void movePlayer(Player&, uWS::WebSocket<false, true, Player>*, char key);
  void playerLeft(Player&, uWS::WebSocket<false, true, Player>*);

  void tick();

private:
  void broadcastPlayerDataOnNextTick();

  void broadcastGameInfo() const;
  void broadcastPlayerData();
  void broadcastPaintMapClear();
  void sendMap(uWS::WebSocket<false, true, Player>*) const;

  void initColors();
	void initMap();
  char getCell(int x, int y) const;
  void paintCell(int x, int y, std::uint8_t clr);
  Player* getPlayerAt(int x, int y) const;

  void resetPlayerScores();
};
