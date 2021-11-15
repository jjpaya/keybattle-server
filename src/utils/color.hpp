#pragma once

#include <cmath>
#include <cstdint>

union RGB_u {
  struct {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a;
  } v;
  std::uint32_t rgba;
};

RGB_u rgb_hue_rotate(RGB_u in, float deg);
