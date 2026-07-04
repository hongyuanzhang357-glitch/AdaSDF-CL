#pragma once

#include <cstdint>
#include <string>

namespace adasdf {

struct CollisionGroupMask {
  std::uint32_t group = 1u;
  std::uint32_t mask = 0xffffffffu;
};

bool canCollide(const CollisionGroupMask& a, const CollisionGroupMask& b);
std::uint32_t parseCollisionMask(const std::string& text);
std::string collisionMaskToString(std::uint32_t value);

}  // namespace adasdf
