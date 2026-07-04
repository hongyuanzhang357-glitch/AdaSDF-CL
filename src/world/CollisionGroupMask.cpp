#include "adasdf/world/CollisionGroupMask.h"

#include <sstream>
#include <stdexcept>

namespace adasdf {

bool canCollide(const CollisionGroupMask& a, const CollisionGroupMask& b) {
  return (a.group & b.mask) != 0u && (b.group & a.mask) != 0u;
}

std::uint32_t parseCollisionMask(const std::string& text) {
  if (text.empty()) {
    throw std::runtime_error("empty collision group/mask value");
  }
  std::size_t parsed = 0;
  const unsigned long value = std::stoul(text, &parsed, 0);
  if (parsed != text.size()) {
    throw std::runtime_error("invalid collision group/mask value: " + text);
  }
  return static_cast<std::uint32_t>(value);
}

std::string collisionMaskToString(std::uint32_t value) {
  std::ostringstream out;
  out << "0x" << std::hex << value;
  return out.str();
}

}  // namespace adasdf
