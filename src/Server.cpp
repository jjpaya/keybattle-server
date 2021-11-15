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
			
			res->template upgrade<Player>(g.getNewPlayer(),
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

    .close = [this] (auto *ws, int, auto) {
      Player * p = ws->getUserData();
      p->g->playerLeft(*p, ws);
      cleanEmptyGames();
    }
	});

  struct us_loop_t *loop = (struct us_loop_t *) uWS::Loop::get();
  struct us_timer_t *delayTimer = us_create_timer(loop, 0, sizeof(void *));
  Server *sv = this;
  std::memcpy(us_timer_ext(delayTimer), &sv, sizeof(void *));
  us_timer_set(delayTimer, [] (struct us_timer_t *t) {
    Server * s = *static_cast<Server**>(us_timer_ext(t));
    s->tickGames();
  }, 50, 50);
}

Game& Server::getIdleAndNonFullGame() {
	for (Game& g : runningGames) {
		if (g.getState() == Game::State::IDLE
        && g.getNumPlayers() < Game::PLAYERS_PER_ROUND) {
			return g;
		}
	}
	
	return runningGames.emplace_back(*static_cast<uWS::App*>(this), ++nextGameId);
}

void Server::tickGames() {
  for (Game& g : runningGames) {
    g.tick();
  }
}

void Server::cleanEmptyGames() {
  for (auto it = runningGames.begin(); it != runningGames.end(); ) {
    if (it->getNumPlayers() == 0) {
      it = runningGames.erase(it);
    } else {
      it++;
    }
  }
}
