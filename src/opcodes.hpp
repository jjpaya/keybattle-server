#pragma once
#include <cstdint>

enum ServerOpCode : std::uint8_t {
  SOP_GAME_INFO = 0,
  SOP_GAME_MAP = 1,
  SOP_GAME_PLAYER_DATA = 2,
  SOP_GAME_SELF_INFO = 3,
  SOP_GAME_WRONG_MOVE = 4
};

enum ClientOpCode : std::uint8_t {
  COP_KEYPRESS = 0
};
