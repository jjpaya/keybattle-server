#pragma once

#include <cstdint>
#include <vector>
#include <App.h>

#include "Game.hpp"

class Server : uWS::App {
	std::uint32_t nextGameId;
	std::uint32_t nextPlayerId;
	std::vector<Game> runningGames;
	
public:
	Server();
	
	Game& getIdleAndNonFullGame();
	
	using uWS::App::publish;
	using uWS::App::listen;
	using uWS::App::run;
};
