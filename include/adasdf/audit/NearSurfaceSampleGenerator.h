#pragma once

#include <cstddef>
#include <vector>

#include "adasdf/geometry/Transform.h"
#include "adasdf/mesh/TriangleMesh.h"

namespace adasdf {

struct NearSurfaceSample {
  std::size_t surface_sample_id = 0;
  int triangle_index = -1;
  Vector3 surface_point;
  Vector3 point;
  Vector3 triangle_normal;
  double offset = 0.0;
};

struct NearSurfaceSampleOptions {
  std::size_t surface_sample_count = 10000;
  std::vector<double> offsets = {-0.004, -0.002, -0.001, 0.0,
                                 0.001, 0.002, 0.004};
  unsigned int seed = 1337;
};

struct NearSurfaceSampleSet {
  std::vector<NearSurfaceSample> samples;
  std::size_t requested_surface_sample_count = 0;
  std::size_t generated_surface_sample_count = 0;
  double total_surface_area = 0.0;
};

class NearSurfaceSampleGenerator {
 public:
  static NearSurfaceSampleSet generate(
      const TriangleMesh& mesh,
      const NearSurfaceSampleOptions& options = NearSurfaceSampleOptions{});
};

}  // namespace adasdf
