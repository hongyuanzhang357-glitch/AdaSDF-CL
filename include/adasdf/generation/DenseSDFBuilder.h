#pragma once

#include <memory>
#include <string>
#include <vector>

#include "adasdf/acceleration/BuildAccelerationReport.h"
#include "adasdf/cache/BuildCacheOptions.h"
#include "adasdf/cache/BuildCacheStats.h"
#include "adasdf/geometry/DenseSDFModel.h"
#include "adasdf/mesh/MeshDiagnostics.h"
#include "adasdf/mesh/MeshReadiness.h"
#include "adasdf/mesh/TriangleMesh.h"

namespace adasdf {

struct DenseSDFBuildOptions {
  int resolution = 64;
  double padding = 0.05;

  bool signed_distance = true;
  bool require_watertight_for_signed = true;

  bool run_mesh_diagnostics = true;
  bool run_readiness_check = true;
  bool auto_safe_cleanup = false;

  double vertex_merge_tolerance = 1e-12;
  double degenerate_area_epsilon = 1e-14;

  SDFSamplingAcceleration acceleration = SDFSamplingAcceleration::BVH;
  int threads = 1;
  bool benchmark_brute_reference = false;
  BuildCacheOptions cache_options;

  bool verbose = true;
};

struct DenseSDFBuildReport {
  bool success = false;
  std::string error_message;

  int nx = 0;
  int ny = 0;
  int nz = 0;

  double padding = 0.0;
  double build_time_ms = 0.0;

  std::size_t triangle_count = 0;
  std::size_t vertex_count = 0;
  std::size_t memory_bytes = 0;

  bool signed_distance = true;
  bool used_cleanup = false;
  bool watertight = false;
  bool used_bvh = false;
  int threads_used = 1;

  BuildAccelerationStats acceleration_stats;
  BuildCacheStats cache_stats;

  MeshDiagnosticsReport diagnostics;
  MeshReadinessReport readiness;

  std::vector<std::string> warnings;
};

class DenseSDFBuilder {
 public:
  static std::shared_ptr<SDFModel> fromMesh(
      const TriangleMesh& mesh,
      const DenseSDFBuildOptions& options,
      DenseSDFBuildReport* report = nullptr);

  static std::shared_ptr<SDFModel> fromSTL(
      const std::string& stl_path,
      const DenseSDFBuildOptions& options,
      DenseSDFBuildReport* report = nullptr);
};

}  // namespace adasdf
