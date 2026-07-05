#pragma once

#include <memory>
#include <string>
#include <vector>

#include "adasdf/acceleration/BuildAccelerationReport.h"
#include "adasdf/generation/AdaptiveOctreeBuilder.h"
#include "adasdf/geometry/AdaptiveBlockSDFModel.h"
#include "adasdf/mesh/MeshDiagnostics.h"
#include "adasdf/mesh/MeshReadiness.h"
#include "adasdf/mesh/TriangleMesh.h"
#include "adasdf/sampling/ContactBandBlockSampler.h"
#include "adasdf/sampling/HierarchicalBlockSampler.h"

namespace adasdf {

struct AdaptiveBlockSDFBuildOptions {
  int min_octree_level = 1;
  int max_octree_level = 5;

  int block_resolution = 8;

  double padding = 0.05;
  double target_near_surface_error = 1e-3;
  double surface_band_factor = 1.5;

  bool signed_distance = true;
  bool require_watertight_for_signed = true;

  bool run_mesh_diagnostics = true;
  bool run_readiness_check = true;
  bool auto_safe_cleanup = false;

  double vertex_merge_tolerance = 1e-12;
  double degenerate_area_epsilon = 1e-14;

  SDFSamplingAcceleration acceleration = SDFSamplingAcceleration::BruteForce;
  int threads = 1;
  bool benchmark_brute_reference = false;

  HierarchicalSamplingOptions hierarchical_sampling;
  ContactBandSamplingOptions contact_band_sampling;

  bool verbose = true;
};

struct AdaptiveBlockSDFBuildReport {
  bool success = false;
  std::string error_message;

  bool signed_distance = true;
  bool watertight = false;
  bool used_cleanup = false;

  int min_octree_level = 0;
  int max_octree_level = 0;
  int max_octree_level_used = 0;
  int block_resolution = 0;

  std::size_t octree_node_count = 0;
  std::size_t octree_leaf_count = 0;
  std::size_t block_count = 0;
  std::size_t near_surface_block_count = 0;

  std::size_t memory_bytes = 0;

  double build_time_ms = 0.0;
  double sampling_time_ms = 0.0;
  bool used_bvh = false;
  int threads_used = 1;
  BuildAccelerationStats acceleration_stats;
  HierarchicalBlockSamplingStats hierarchical_sampling;
  ContactBandDiagnostics contact_band_sampling;

  MeshDiagnosticsReport diagnostics;
  MeshReadinessReport readiness;
  AdaptiveOctreeBuildReport octree_report;

  std::vector<std::string> warnings;
};

class AdaptiveBlockSDFBuilder {
 public:
  static std::shared_ptr<SDFModel> fromMesh(
      const TriangleMesh& mesh,
      const AdaptiveBlockSDFBuildOptions& options,
      AdaptiveBlockSDFBuildReport* report = nullptr);

  static std::shared_ptr<SDFModel> fromSTL(
      const std::string& stl_path,
      const AdaptiveBlockSDFBuildOptions& options,
      AdaptiveBlockSDFBuildReport* report = nullptr);
};

}  // namespace adasdf
