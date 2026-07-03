#include <adasdf/adasdf.h>

#include <cmath>
#include <iostream>
#include <vector>

namespace {

adasdf::ContactCandidate candidate(int id, double x, double depth) {
  adasdf::ContactCandidate c;
  c.sample_id = id;
  c.point = {x, 0.0, 0.0};
  c.penetration_depth = depth;
  c.has_normal = true;
  c.normal = {1.0, 0.0, 0.0};
  return c;
}

}  // namespace

int main() {
  auto patch = adasdf::ContactPatchBuilder::fromMembers(
      7,
      {candidate(2, 0.0, 0.2), candidate(1, 2.0, 0.4)});
  if (patch.patch_id != 7 || patch.members.size() != 2) {
    std::cerr << "patch metadata failed\n";
    return 1;
  }
  if (std::abs(patch.centroid.x - 1.0) > 1e-12 ||
      !patch.average_normal.allFinite()) {
    std::cerr << "patch centroid or normal failed\n";
    return 1;
  }
  if (patch.representative_sample_id != 1 ||
      std::abs(patch.max_penetration_depth - 0.4) > 1e-12) {
    std::cerr << "patch representative failed\n";
    return 1;
  }
  auto empty = adasdf::ContactPatchBuilder::fromMembers(1, {});
  if (!empty.members.empty() || empty.representative_sample_id != -1) {
    std::cerr << "empty patch failed\n";
    return 1;
  }
  std::cout << "contact patch passed\n";
  return 0;
}
