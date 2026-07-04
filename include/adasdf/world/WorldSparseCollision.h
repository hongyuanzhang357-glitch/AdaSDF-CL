#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/sparse/SparseCollisionQuery.h"
#include "adasdf/world/AABBBroadphase.h"

namespace adasdf {

struct WorldSparseCollisionOptions {
  SparseCollisionMode mode = SparseCollisionMode::CollisionOnly;
  double threshold = 0.0;
  bool bidirectional = true;
  bool early_exit = true;
  bool compute_normals = false;
  bool use_sample_radius = true;
  bool return_all_violations = true;
  AABBBroadphaseOptions broadphase_options;
};

struct WorldSparseSampleResult {
  int pair_id = -1;
  int source_object_id = -1;
  int target_object_id = -1;
  std::string source_name;
  std::string target_name;
  SparseSDFSampleResult sample;
  Vector3 world_position;
  Vector3 world_normal;
  bool has_world_normal = false;
};

struct WorldSparsePairResult {
  int pair_id = -1;
  int object_a = -1;
  int object_b = -1;
  std::string name_a;
  std::string name_b;
  bool colliding = false;
  double min_phi = 0.0;
  double min_effective_phi = 0.0;
  std::size_t sample_count = 0;
  std::size_t queried_count = 0;
  std::size_t violation_count = 0;
  bool early_exit_triggered = false;
  std::vector<WorldSparseSampleResult> violations;
};

struct WorldSparseCollisionStats {
  std::size_t broadphase_pair_count = 0;
  std::size_t queried_pair_count = 0;
  std::size_t sample_count = 0;
  std::size_t queried_sample_count = 0;
  std::size_t violation_count = 0;
  double min_phi = 0.0;
  double min_effective_phi = 0.0;
  double elapsed_ms = 0.0;
};

struct WorldSparseCollisionResult {
  bool success = false;
  std::string error_message;
  bool colliding = false;
  AABBBroadphaseResult broadphase;
  std::vector<WorldSparsePairResult> pairs;
  WorldSparseCollisionStats stats;
  std::vector<std::string> warnings;
};

class WorldSparseCollision {
 public:
  static WorldSparseCollisionResult check(
      const CollisionWorld& world,
      const WorldSparseCollisionOptions& options = WorldSparseCollisionOptions{});
};

const char* toString(WorldSparseCollisionOptions);

}  // namespace adasdf
