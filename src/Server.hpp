#pragma once

#include <cstdint>
#include <list>
#include <App.h>

#include "Game.hpp"

class Server : uWS::App {
	std::uint32_t nextGameId;
	std::uint32_t nextPlayerId;
	std::list<Game> runningGames;
	
public:
	Server();
	
	Game& getIdleAndNonFullGame();

  void tickGames();
  void cleanEmptyGames();
	
	using uWS::App::publish;
	using uWS::App::listen;
	using uWS::App::run;
};
