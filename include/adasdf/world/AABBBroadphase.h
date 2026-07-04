#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/world/CollisionWorld.h"

namespace adasdf {

struct AABBBroadphaseOptions {
  bool include_disabled = false;
  bool include_static_static = false;
  bool use_group_mask = true;
  double aabb_margin = 0.0;
};

struct WorldPair {
  int pair_id = -1;
  int object_a = -1;
  int object_b = -1;
  std::string name_a;
  std::string name_b;
  WorldObjectType type_a = WorldObjectType::Dynamic;
  WorldObjectType type_b = WorldObjectType::Dynamic;
  AABB aabb_a;
  AABB aabb_b;
  bool group_mask_allowed = true;
  bool aabb_overlap = false;
};

struct AABBBroadphaseStats {
  std::size_t object_count = 0;
  std::size_t tested_pair_count = 0;
  std::size_t disabled_pair_skipped = 0;
  std::size_t static_static_pair_skipped = 0;
  std::size_t group_mask_pair_skipped = 0;
  std::size_t invalid_aabb_pair_skipped = 0;
  std::size_t aabb_rejected_count = 0;
  std::size_t overlap_pair_count = 0;
  double elapsed_ms = 0.0;
};

struct AABBBroadphaseResult {
  bool success = false;
  std::string error_message;
  std::vector<WorldPair> pairs;
  AABBBroadphaseStats stats;
};

class AABBBroadphase {
 public:
  static AABBBroadphaseResult compute(
      const CollisionWorld& world,
      const AABBBroadphaseOptions& options = AABBBroadphaseOptions{});
};

bool overlaps(const AABB& a, const AABB& b, double margin = 0.0);

}  // namespace adasdf
