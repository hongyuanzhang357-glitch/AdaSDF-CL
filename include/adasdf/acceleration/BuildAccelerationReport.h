#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace adasdf {

enum class SDFSamplingAcceleration {
  BruteForce,
  BVH,
};

const char* toString(SDFSamplingAcceleration acceleration);
bool parseSDFSamplingAcceleration(
    const std::string& text,
    SDFSamplingAcceleration* acceleration);

struct BuildAccelerationStats {
  SDFSamplingAcceleration acceleration = SDFSamplingAcceleration::BruteForce;
  bool used_bvh = false;
  int threads_requested = 1;
  int threads_used = 1;

  std::size_t bvh_node_count = 0;
  std::size_t bvh_leaf_count = 0;
  std::size_t bvh_triangle_count = 0;
  std::size_t sample_count = 0;
  std::size_t nearest_triangle_tests = 0;
  std::size_t nearest_node_visits = 0;
  std::size_t ray_triangle_tests = 0;
  std::size_t ray_node_visits = 0;
  std::size_t ambiguous_sign_count = 0;
  std::size_t fallback_count = 0;

  double bvh_build_time_ms = 0.0;
  double sampling_time_ms = 0.0;
  double brute_reference_time_ms = 0.0;
  double speedup_vs_bruteforce = 0.0;

  std::vector<std::string> warnings;
};

std::string toMarkdown(const BuildAccelerationStats& stats);
std::string toJson(const BuildAccelerationStats& stats);

}  // namespace adasdf
