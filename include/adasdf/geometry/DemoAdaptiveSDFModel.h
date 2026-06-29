#pragma once

#include <memory>
#include <string>
#include <vector>

#include "adasdf/generation/BuildOptions.h"
#include "adasdf/geometry/SDFModel.h"

namespace adasdf {

struct DemoOctreeNodeInfo {
  int level = 0;
  Vector3 min_corner;
  Vector3 max_corner;
  bool near_surface = false;
};

struct DemoBlockInfo {
  int block_id = 0;
  Vector3 min_corner;
  Vector3 max_corner;
  int resolution = 16;
  int rank = 4;
  double estimated_error = 0.0;
  double estimated_memory_kb = 0.0;
};

struct DemoAdaptiveSDFDescription {
  std::string shape_type = "box";
  Vector3 center = Vector3::Zero();
  Vector3 half_extent{0.5, 0.5, 0.5};
  std::string unit = "m";
  double target_near_surface_error = 1.0e-3;
  double memory_limit_mb = 64.0;
  double block_expand_limit_mb = 16.0;
  std::string surrogate_id = "demo_v0_9";
  std::string warning = "demo_surrogate_not_universal";
  BuildOptions build_options;
};

class DemoAdaptiveSDFModel final : public SDFModel {
 public:
  DemoAdaptiveSDFModel(
      DemoAdaptiveSDFDescription description,
      std::vector<DemoOctreeNodeInfo> octree_nodes,
      std::vector<DemoBlockInfo> blocks);

  static std::shared_ptr<DemoAdaptiveSDFModel> create(
      DemoAdaptiveSDFDescription description,
      std::vector<DemoOctreeNodeInfo> octree_nodes,
      std::vector<DemoBlockInfo> blocks);

  const DemoAdaptiveSDFDescription& description() const {
    return description_;
  }

  const std::vector<DemoOctreeNodeInfo>& octreeNodes() const {
    return octree_nodes_;
  }

  const std::vector<DemoBlockInfo>& blocks() const {
    return blocks_;
  }

  const Vector3& center() const {
    return description_.center;
  }

  const Vector3& halfExtent() const {
    return description_.half_extent;
  }

  const std::string& unit() const {
    return description_.unit;
  }

  std::string shapeName() const {
    return description_.shape_type;
  }

  void setSourcePath(std::string source_path);

  static const char* backendName();
  static const char* formatMagic();

 private:
  DemoAdaptiveSDFDescription description_;
  std::vector<DemoOctreeNodeInfo> octree_nodes_;
  std::vector<DemoBlockInfo> blocks_;
};

}  // namespace adasdf
