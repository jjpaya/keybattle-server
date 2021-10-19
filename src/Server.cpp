#include "Server.hpp"
#include "Player.hpp"
#include "WebSocket.h"

using App = uWS::App;

Server::Server() {
	App::get("/hello", [] (auto *res, auto *req) {
		res->end("hello");
	});
	
	App::ws<Player>("/",{
		.closeOnBackpressureLimit = true,
		
		.upgrade = [this] (auto *res, auto *req, auto *context) {
			Game& g = getIdleAndNonFullGame();
			
			res->template upgrade<Player>({},
					req->getHeader("sec-websocket-key"),
					req->getHeader("sec-websocket-protocol"),
					req->getHeader("sec-websocket-extensions"),
					context);
		},
		
		.open = [] (auto *ws) {
      Player *p = ws->getUserData();
			//ws->subscribe();
		}
	});
}

Game& Server::getIdleAndNonFullGame() {
	for (Game& g : runningGames) {
		if (g.isIdle() && g.getNumPlayers() < Game::PLAYERS_PER_ROUND) {
			return g;
		}
	}
	
	return runningGames.emplace_back(*static_cast<uWS::App*>(this), ++nextGameId);
}
