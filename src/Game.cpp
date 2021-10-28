#include "Game.hpp"
#include <algorithm>
#include <cstring>
#include <charconv>
#include <exception>
#include <initializer_list>
#include <string_view>
#include <system_error>

#include "Player.hpp"
#include "WebSocket.h"
#include "opcodes.hpp"

Game::Game(uWS::App& app, std::uint32_t id)
: gameId(id),
  app(app) {
  initMap();
}

std::string_view Game::getTopicName() const {
  static char gameTopic[12] = "g/";

	if (auto [ptr, ec] = std::to_chars(std::begin(gameTopic) + 2, std::end(gameTopic), gameId);
			ec == std::errc()) {
		return {gameTopic, static_cast<std::size_t>(std::distance(gameTopic, ptr))};
	}
	
  return {};
}

uWS::Topic * Game::getTopic() const {
  return app.topicTree ? app.topicTree->lookupTopic(getTopicName()) : nullptr;
}

bool Game::isIdle() const {
  return true;
}

std::uint32_t Game::getNumPlayers() const {
  return app.numSubscribers(getTopicName());
}

void Game::newPlayer(Player& p, uWS::WebSocket<false, true, Player>* ws) {
  std::array<char, 1 + 4> self{SOP_GAME_SELF_INFO};
  std::memcpy(&self[1], &p.id, 4);
  ws->send({self.data(), self.size()});
	ws->subscribe(getTopicName());
  broadcastGameInfo();
  sendMap(ws);
  broadcastPlayerData();
}

void Game::movePlayer(Player& p, uWS::WebSocket<false, true, Player>* ws, char key) {
  if (key == '\0') {
    return;
  }

  for (int offY = -1; offY <= 1; offY++) {
    for (int offX = -1; offX <= 1; offX++) {
      char c = getCell(p.x + offX, p.y + offY);
      if (c == key) {
        p.x += offX;
        p.y += offY;
        broadcastPlayerData();
        return;
      }
    }
  }

  std::array<char, 1> bad{SOP_GAME_WRONG_MOVE};
  ws->send({bad.data(), bad.size()});
}

void Game::playerLeft(Player& p, uWS::WebSocket<false, true, Player>* ws) {
  ws->unsubscribe(getTopicName());
  broadcastPlayerData();
  broadcastGameInfo();
}

void Game::broadcastGameInfo() const {
  std::array<char, 1
      + sizeof(std::uint32_t/*gameid*/)
      + sizeof(int/*mapWidth*/)
      + sizeof(std::uint8_t/*numPlayers*/)> buf;
  buf[0] = SOP_GAME_INFO;
  std::memcpy(&buf[1], &gameId, sizeof(std::uint32_t));
  std::memcpy(&buf[5], &MAP_WIDTH, sizeof(int));
  buf[9] = std::min(getNumPlayers(), 255u);

  app.publish(getTopicName(), {buf.data(), buf.size()}, uWS::OpCode::BINARY);
}

void Game::broadcastPlayerData() const {
  if (getNumPlayers() == 0) {
    return;
  }

  std::vector <char> buf(1 + getNumPlayers() * 12);
  buf[0] = SOP_GAME_PLAYER_DATA;
  std::size_t off = 1;
  for (auto * s : *getTopic()) {
    /* super disgusting */
    Player * p = static_cast<uWS::WebSocket<false, true, Player>*>(s->user)->getUserData();
    std::memcpy(&buf[off], &p->id, 4);
    std::memcpy(&buf[off + 4], &p->x, 4);
    std::memcpy(&buf[off + 8], &p->y, 4);
    off += 12;
  }

  app.publish(getTopicName(), {buf.data(), buf.size()}, uWS::OpCode::BINARY);
}

void Game::sendMap(uWS::WebSocket<false, true, Player> * ws) const {
  std::array<char, 1 + std::tuple_size_v<decltype(map)>> buf;

  buf[0] = SOP_GAME_MAP;
  std::copy(map.begin(), map.end(), buf.begin() + 1);

  ws->send({buf.data(), buf.size()});
}



void Game::initMap() {
	static constexpr std::initializer_list<char> keys {
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
		'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
		'Z', 'X', 'C', 'V', 'B', 'N', 'M'
	};

  static_assert(keys.size() >= 5 * 5, "Minimum number of key characters are 25");

  auto getAvailable = [this] (int x, int y) -> const std::vector<char>& {
    /* 5*5 - 1 = max number of excluded chars, -1 to not count current cell */
    static std::vector<char> availKeys(keys.size());
    static std::vector<char> exclude(5 * 5 - 1);
    availKeys = keys;
    exclude.clear();
    
    for (int offY = -2; offY <= 2; offY++) {
      for (int offX = -2; offX <= 2; offX++) {
        char c = getCell(x + offX, y + offY);
        /* If we're not in the center cell and the cell has a value */
        if (c != '\0' && !(offX == 0 && offY == 0)) {
          exclude.push_back(c);
        }
      }
    }

    /* sort everything */
    std::sort(availKeys.begin(), availKeys.end());
    std::sort(exclude.begin(), exclude.end());

    /* and keep only elements that won't repeat */
    availKeys.erase(std::remove_if(availKeys.begin(), availKeys.end(), [] (char x) {
      return std::binary_search(exclude.begin(), exclude.end(), x);
    }), availKeys.end());

    return availKeys;
  };

  auto genCell = [&getAvailable](int x, int y) -> char {
    const std::vector<char>& avail = getAvailable(x, y);
    if (avail.empty()) {
      return '\0';
    }

    return avail[rng() % avail.size()];
  };

  map.fill('\0');

  for (std::size_t y = 0; y < MAP_HEIGHT; y++) {
    for (std::size_t x = 0; x < MAP_WIDTH; x++) {
      map[y * MAP_WIDTH + x] = genCell(x, y);
    }
	}
}

char Game::getCell(int x, int y) const {
  return x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT ? '\0' : map[y * MAP_WIDTH + x];
}
