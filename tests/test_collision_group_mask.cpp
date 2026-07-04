#include <adasdf/adasdf.h>

#include <iostream>

int main() {
  adasdf::CollisionGroupMask robot{0x1u, 0x2u};
  adasdf::CollisionGroupMask world{0x2u, 0x1u};
  adasdf::CollisionGroupMask hidden{0x4u, 0x8u};
  if (!adasdf::canCollide(robot, world)) {
    std::cerr << "compatible group/mask pair rejected\n";
    return 1;
  }
  if (adasdf::canCollide(robot, hidden)) {
    std::cerr << "incompatible group/mask pair accepted\n";
    return 1;
  }
  if (adasdf::parseCollisionMask("0x10") != 16u ||
      adasdf::parseCollisionMask("7") != 7u) {
    std::cerr << "collision mask parser failed\n";
    return 1;
  }
  std::cout << "collision group mask passed\n";
  return 0;
}
