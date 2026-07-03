#include <adasdf/adasdf.h>

#include <iostream>
#include <vector>

namespace {

adasdf::ContactCandidate candidate(
    int id,
    double x,
    double depth,
    adasdf::Vector3 normal) {
  adasdf::ContactCandidate c;
  c.sample_id = id;
  c.point = {x, 0.0, 0.0};
  c.penetration_depth = depth;
  c.normal = normal;
  c.has_normal = true;
  return c;
}

}  // namespace

int main() {
  adasdf::ContactPatchOptions options;
  options.spatial_radius = 0.05;
  auto clustered = adasdf::ContactClusterer::cluster(
      {candidate(1, 0.0, 0.4, {1.0, 0.0, 0.0}),
       candidate(2, 0.01, 0.3, {1.0, 0.0, 0.0})},
      options);
  if (clustered.patch_count != 1 || clustered.patches[0].patch_id != 0) {
    std::cerr << "near candidates should share patch\n";
    return 1;
  }

  clustered = adasdf::ContactClusterer::cluster(
      {candidate(1, 0.0, 0.4, {1.0, 0.0, 0.0}),
       candidate(2, 1.0, 0.3, {1.0, 0.0, 0.0})},
      options);
  if (clustered.patch_count != 2) {
    std::cerr << "far candidates should split patches\n";
    return 1;
  }

  clustered = adasdf::ContactClusterer::cluster(
      {candidate(1, 0.0, 0.4, {1.0, 0.0, 0.0}),
       candidate(2, 0.01, 0.3, {-1.0, 0.0, 0.0})},
      options);
  if (clustered.patch_count != 2) {
    std::cerr << "inconsistent normals should split patches\n";
    return 1;
  }

  options.require_normal_consistency = false;
  clustered = adasdf::ContactClusterer::cluster(
      {candidate(1, 0.0, 0.4, {1.0, 0.0, 0.0}),
       candidate(2, 0.01, 0.3, {-1.0, 0.0, 0.0})},
      options);
  if (clustered.patch_count != 1) {
    std::cerr << "normal consistency off should cluster spatially\n";
    return 1;
  }

  std::cout << "contact clusterer passed\n";
  return 0;
}
