#include "Game.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <charconv>
#include <exception>
#include <initializer_list>
#include <string_view>
#include <system_error>

#include "utils/color.hpp"

#include "Player.hpp"
#include "WebSocket.h"
#include "opcodes.hpp"

template<typename Timepoint>
std::int64_t js_time(Timepoint tp) {
	namespace c = std::chrono;

	auto time = tp.time_since_epoch();
	return c::duration_cast<c::milliseconds>(time).count();
}

using WebSocket = uWS::WebSocket<false, true, Player>;
using sclock = std::chrono::system_clock;

Game::Game(uWS::App& app, std::uint32_t id)
: gameId(id),
  app(app),
  state(State::IDLE),
  stateTime(std::chrono::system_clock::now()),
  sendPlayerDataOnTick(false) {
  initColors();
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

void Game::setState(State s) {
  state = s;
  stateTime = std::chrono::system_clock::now();
  broadcastGameInfo();
}

Game::State Game::getState() const {
  return state;
}

std::uint32_t Game::getNumPlayers() const {
  return app.numSubscribers(getTopicName());
}

Player Game::getNewPlayer() {
  int x = rng() % (Game::MAP_WIDTH - 8) + 4;
  int y = rng() % (Game::MAP_HEIGHT - 8) + 4;
  return Player{this, playerIds.getId(), x, y, 0, sclock::time_point::min()};
}

void Game::newPlayer(Player& p, WebSocket* ws) {
  if (getNumPlayers() < 2 && getState() == State::IDLE) {
    stateTime = std::chrono::system_clock::now();
  }

  std::array<char, 1 + 4> self{SOP_GAME_SELF_INFO};
  std::memcpy(&self[1], &p.id, 4);
  ws->send({self.data(), self.size()});
	ws->subscribe(getTopicName());
  broadcastGameInfo();
  //paintCell(p.x, p.y, p.id);
  sendMap(ws);
  broadcastPlayerDataOnNextTick();
}

void Game::movePlayer(Player& p, WebSocket* ws, char key) {
  if (key == '\0' || p.frozenUntil != sclock::time_point::min()) {
    return;
  }

  for (int offY = -1; offY <= 1; offY++) {
    for (int offX = -1; offX <= 1; offX++) {
      char c = getCell(p.x + offX, p.y + offY);
      Player * pl = getPlayerAt(p.x + offX, p.y + offY);
      if (c == key) {
        if (pl) { /* push the player! */
          char c = getCell(pl->x + offX, pl->y + offY);
          Player * pl2 = getPlayerAt(pl->x + offX, pl->y + offY);
          if (c != '\0' && !pl2) { /* but don't push off the map, or into another player */
            pl->x += offX;
            pl->y += offY;
          } else { /* else block the movement */
            return;
          }
        }

        p.x += offX;
        p.y += offY;
        
        if (getState() != State::ENDED) {
          paintCell(p.x, p.y, p.id);
        }
        
        broadcastPlayerDataOnNextTick();
        return;
      }
    }
  }

  std::array<char, 1> bad{SOP_GAME_WRONG_MOVE};
  ws->send({bad.data(), bad.size()});

  if (getState() == State::PLAYING) {
    p.points -= 35;
  }

  p.frozenUntil = sclock::now() + std::chrono::seconds(2);
  broadcastPlayerDataOnNextTick();
}

void Game::playerLeft(Player& p, WebSocket* ws) {
  playerIds.freeId(p.id);
  ws->unsubscribe(getTopicName());
  broadcastPlayerDataOnNextTick();
  broadcastGameInfo();
}

void Game::tick() {
  constexpr auto min = sclock::time_point::min();
  auto now = sclock::now();

  /* check player frozen status and send update if any changed */
  for (auto * s : *getTopic()) {
    Player * p = static_cast<WebSocket*>(s->user)->getUserData();
    if (p->frozenUntil != min && now > p->frozenUntil) {
      p->frozenUntil = min;
      sendPlayerDataOnTick = true;
    }
  }

  if (sendPlayerDataOnTick) {
    sendPlayerDataOnTick = false;
    broadcastPlayerData();
  }

  if (getState() == State::IDLE && getNumPlayers() < 2) {
    return;
  }

  switch (getState()) {
    case State::IDLE:
      if (now - stateTime > std::chrono::seconds(30)) {
        paintMap.fill(0);
        broadcastPaintMapClear();
        setState(State::PLAYING);
      }
      break;

    case State::PLAYING:
      if (now - stateTime > std::chrono::seconds(std::min(60 * (getNumPlayers() / 2), 300u))) {
        setState(State::ENDED);
      }
      break;

    case State::ENDED:
      if (now - stateTime > std::chrono::seconds(30)) {
        paintMap.fill(0);
        broadcastPaintMapClear();
        setState(State::IDLE);
        resetPlayerScores();
      }
      break;
  }
}

void Game::broadcastPlayerDataOnNextTick() {
  sendPlayerDataOnTick = true;
}

void Game::broadcastGameInfo() const {
  std::int64_t intStateTime = js_time(stateTime);

  std::array<std::uint8_t, 1
      + sizeof(std::uint8_t/*numPlayers*/)
      + sizeof(std::uint16_t/*state*/)
      + sizeof(std::uint32_t/*gameid*/)
      + sizeof(int/*mapWidth*/)
      + sizeof(std::uint32_t/*PLAYERS_PER_ROUND*/)
      + sizeof(std::int64_t/*stateTime*/)
      + sizeof(std::uint32_t) * std::tuple_size_v<decltype(playerColors)>> buf;
  buf[0] = SOP_GAME_INFO;
  buf[1] = std::min(getNumPlayers(), 255u);
  std::memcpy(&buf[2], &state, sizeof(state));
  std::memcpy(&buf[4], &gameId, sizeof(std::uint32_t));
  std::memcpy(&buf[8], &MAP_WIDTH, sizeof(int));
  std::memcpy(&buf[12], &PLAYERS_PER_ROUND, sizeof(std::uint32_t));
  std::memcpy(&buf[16], &intStateTime, sizeof(std::int64_t));
  
  std::memcpy(&buf[24], playerColors.begin(), playerColors.size() * sizeof(std::uint32_t));

  app.publish(getTopicName(), {reinterpret_cast<char*>(buf.data()), buf.size()}, uWS::OpCode::BINARY);
}

void Game::broadcastPlayerData() {
  constexpr auto min = sclock::time_point::min();
  auto now = sclock::now();

  if (getNumPlayers() == 0) {
    paintUpdates.clear();
    return;
  }

  std::vector <char> buf(1 + 1 + getNumPlayers() * 17 + 2 + paintUpdates.size() * 9);
  buf[0] = SOP_GAME_PLAYER_DATA;
  buf[1] = getNumPlayers(); /* breaks if >255 players */
  std::size_t off = 2;
  for (auto * s : *getTopic()) {
    /* not pretty */
    Player * p = static_cast<WebSocket*>(s->user)->getUserData();
    bool frozen = p->frozenUntil != min && now < p->frozenUntil;

    std::memcpy(&buf[off], &p->id, 4);
    std::memcpy(&buf[off + 4], &p->x, 4);
    std::memcpy(&buf[off + 8], &p->y, 4);
    std::memcpy(&buf[off + 12], &p->points, 4);
    std::memcpy(&buf[off + 16], &frozen, 1);
    off += 17;
  }

  std::uint16_t numUpdates = paintUpdates.size();
  std::memcpy(&buf[off], &numUpdates, 2);
  off += 2;
  for (const PaintEvent& upd : paintUpdates) {
    std::memcpy(&buf[off], &upd.x, 4);
    std::memcpy(&buf[off + 4], &upd.y, 4);
    buf[off + 8] = upd.color;
    off += 9;
  }

  paintUpdates.clear();

  app.publish(getTopicName(), {buf.data(), buf.size()}, uWS::OpCode::BINARY);
}

void Game::broadcastPaintMapClear() {
  std::array<char, 1> buf{SOP_GAME_PAINT_CLEAR};
  app.publish(getTopicName(), {buf.data(), buf.size()}, uWS::OpCode::BINARY);
}

void Game::sendMap(WebSocket * ws) const {
  constexpr auto map_s = std::tuple_size_v<decltype(map)>;
  constexpr auto pmap_s = std::tuple_size_v<decltype(paintMap)>;
  std::array<char, 1 + map_s + pmap_s> buf;

  buf[0] = SOP_GAME_MAP;
  std::copy(map.begin(), map.end(), buf.begin() + 1);
  std::copy(paintMap.begin(), paintMap.end(), buf.begin() + 1 + map_s);

  ws->send({buf.data(), buf.size()});
}

void Game::initColors() {
  constexpr float increment = 360.f / (float)PLAYERS_PER_ROUND;
  float initialHueDeg = rng() % 360;
  RGB_u clr {{63, 192, 63, 255}};

  for (std::size_t i = 0; i < playerColors.size(); i++) {
    float hue = std::fmod(initialHueDeg + increment * i, 360.f);
    playerColors[i] = rgb_hue_rotate(clr, hue).rgba;
    // std::cout << hue << ' ' << std::hex << playerColors[i] << std::dec << std::endl;
  }

  std::shuffle(playerColors.begin(), playerColors.end(), rng);
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
  paintMap.fill(0);

  for (std::size_t y = 0; y < MAP_HEIGHT; y++) {
    for (std::size_t x = 0; x < MAP_WIDTH; x++) {
      map[y * MAP_WIDTH + x] = genCell(x, y);
    }
	}
}

char Game::getCell(int x, int y) const {
  return x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT ? '\0' : map[y * MAP_WIDTH + x];
}

Player* Game::getPlayerAt(int x, int y) const {
  if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT) {
    return nullptr;
  }

  for (auto * s : *getTopic()) {
    Player * p = static_cast<WebSocket*>(s->user)->getUserData();
    if (x == p->x && y == p->y) {
      return p;
    }
  }

  return nullptr;
}

void Game::paintCell(int x, int y, std::uint8_t clr) {
  if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT || paintMap[x + y * MAP_WIDTH] == clr) {
    return;
  }

  paintMap[x + y * MAP_WIDTH] = clr;
  paintUpdates.push_back({x, y, clr});
  broadcastPlayerDataOnNextTick();
}

void Game::resetPlayerScores() {
  for (auto * s : *getTopic()) {
    Player * p = static_cast<WebSocket*>(s->user)->getUserData();
    p->points = 0;
  }

  broadcastPlayerDataOnNextTick();
}
