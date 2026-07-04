#include "adasdf/world/AABBBroadphase.h"

#include <algorithm>
#include <chrono>

namespace adasdf {
namespace {

AABB expanded(AABB box, double margin) {
  if (!box.valid || margin == 0.0) {
    return box;
  }
  box.min.x -= margin;
  box.min.y -= margin;
  box.min.z -= margin;
  box.max.x += margin;
  box.max.y += margin;
  box.max.z += margin;
  return box;
}

}  // namespace

bool overlaps(const AABB& a, const AABB& b, double margin) {
  if (!a.valid || !b.valid) {
    return false;
  }
  const AABB aa = expanded(a, margin);
  const AABB bb = expanded(b, margin);
  return aa.min.x <= bb.max.x && aa.max.x >= bb.min.x &&
      aa.min.y <= bb.max.y && aa.max.y >= bb.min.y &&
      aa.min.z <= bb.max.z && aa.max.z >= bb.min.z;
}

AABBBroadphaseResult AABBBroadphase::compute(
    const CollisionWorld& world,
    const AABBBroadphaseOptions& options) {
  const auto start = std::chrono::steady_clock::now();
  AABBBroadphaseResult result;
  result.stats.object_count = world.objectCount();
  const std::vector<WorldObject>& objects = world.objects();

  for (std::size_t i = 0; i < objects.size(); ++i) {
    for (std::size_t j = i + 1; j < objects.size(); ++j) {
      const WorldObject& a = objects[i];
      const WorldObject& b = objects[j];
      ++result.stats.tested_pair_count;

      if (!options.include_disabled && (!a.enabled || !b.enabled)) {
        ++result.stats.disabled_pair_skipped;
        continue;
      }
      if (!options.include_static_static && isStatic(a) && isStatic(b)) {
        ++result.stats.static_static_pair_skipped;
        continue;
      }
      const bool group_allowed =
          !options.use_group_mask || canCollide(a.group_mask, b.group_mask);
      if (!group_allowed) {
        ++result.stats.group_mask_pair_skipped;
        continue;
      }
      if (!a.world_aabb.valid || !b.world_aabb.valid) {
        ++result.stats.invalid_aabb_pair_skipped;
        continue;
      }
      const bool aabb_overlap =
          overlaps(a.world_aabb, b.world_aabb, options.aabb_margin);
      if (!aabb_overlap) {
        ++result.stats.aabb_rejected_count;
        continue;
      }

      WorldPair pair;
      pair.pair_id = static_cast<int>(result.pairs.size());
      pair.object_a = a.object_id;
      pair.object_b = b.object_id;
      pair.name_a = worldObjectDisplayName(a);
      pair.name_b = worldObjectDisplayName(b);
      pair.type_a = a.type;
      pair.type_b = b.type;
      pair.aabb_a = a.world_aabb;
      pair.aabb_b = b.world_aabb;
      pair.group_mask_allowed = group_allowed;
      pair.aabb_overlap = aabb_overlap;
      result.pairs.push_back(pair);
    }
  }

  std::sort(
      result.pairs.begin(),
      result.pairs.end(),
      [](const WorldPair& a, const WorldPair& b) {
        if (a.object_a != b.object_a) {
          return a.object_a < b.object_a;
        }
        return a.object_b < b.object_b;
      });
  for (std::size_t i = 0; i < result.pairs.size(); ++i) {
    result.pairs[i].pair_id = static_cast<int>(i);
  }

  result.stats.overlap_pair_count = result.pairs.size();
  result.stats.elapsed_ms =
      std::chrono::duration<double, std::milli>(
          std::chrono::steady_clock::now() - start)
          .count();
  result.success = true;
  return result;
}

}  // namespace adasdf
