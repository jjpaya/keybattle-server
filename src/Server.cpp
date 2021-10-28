#include "Server.hpp"
#include "Player.hpp"
#include "opcodes.hpp"

using App = uWS::App;

Server::Server()
: nextGameId(0),
  nextPlayerId(0) {
	App::ws<Player>("/",{
		.closeOnBackpressureLimit = true,
		
		.upgrade = [this] (auto *res, auto *req, auto *context) {
			Game& g = getIdleAndNonFullGame();
			
			res->template upgrade<Player>(Player{&g, ++nextPlayerId, Game::MAP_WIDTH / 2, Game::MAP_HEIGHT / 2},
					req->getHeader("sec-websocket-key"),
					req->getHeader("sec-websocket-protocol"),
					req->getHeader("sec-websocket-extensions"),
					context);
		},
		
		.open = [] (auto *ws) {
      Player *p = ws->getUserData();
      p->g->newPlayer(*p, ws);
		},
    
    .message = [] (auto *ws, std::string_view msg, auto oc) {
      Player * p = ws->getUserData();
      if (msg.size() < 2) {
        ws->close();
      }

      switch (msg[0]) {
        case COP_KEYPRESS:
          p->g->movePlayer(*p, ws, msg[1]);
          break;
      }
    },

    .close = [] (auto *ws, int, auto) {
      Player * p = ws->getUserData();
      p->g->playerLeft(*p, ws);
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
