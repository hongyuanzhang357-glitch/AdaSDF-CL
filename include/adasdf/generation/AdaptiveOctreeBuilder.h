#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/acceleration/BuildAccelerationReport.h"
#include "adasdf/generation/AdaptiveOctree.h"
#include "adasdf/mesh/TriangleMesh.h"

namespace adasdf {

enum class AdaptiveLeafMode {
  Mixed,
  Uniform,
};

const char* toString(AdaptiveLeafMode mode);
bool parseAdaptiveLeafMode(const std::string& value, AdaptiveLeafMode* mode);

struct AdaptiveOctreeBuildOptions {
  int min_level = 1;
  int max_level = 5;

  AdaptiveLeafMode leaf_mode = AdaptiveLeafMode::Mixed;

  double padding = 0.05;

  double target_near_surface_error = 1e-3;
  double surface_band_factor = 1.5;

  bool signed_distance = true;
  bool require_watertight_for_signed = true;

  bool refine_near_surface = true;
  bool refine_sign_changes = true;

  int samples_per_cell_axis = 2;

  SDFSamplingAcceleration distance_backend = SDFSamplingAcceleration::BruteForce;
  double degenerate_area_epsilon = 1e-14;
};

struct AdaptiveOctreeBuildReport {
  bool success = false;
  std::string error_message;

  int min_level = 0;
  int max_level = 0;
  int max_level_used = 0;
  AdaptiveLeafMode leaf_mode = AdaptiveLeafMode::Mixed;

  std::size_t node_count = 0;
  std::size_t leaf_count = 0;
  std::size_t near_surface_leaf_count = 0;

  double build_time_ms = 0.0;

  std::vector<std::string> warnings;
};

class AdaptiveOctreeBuilder {
 public:
  static AdaptiveOctree build(
      const TriangleMesh& mesh,
      const AdaptiveOctreeBuildOptions& options,
      AdaptiveOctreeBuildReport* report = nullptr);
};

}  // namespace adasdf
