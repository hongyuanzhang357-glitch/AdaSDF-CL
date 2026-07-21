#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/generation/SDFCreationContract.h"
#include "adasdf/mesh/MeshDiagnostics.h"

namespace adasdf {

inline constexpr const char* kGeometryFeatureSchema =
    "adasdf.geometry_features.v2";

struct GeometryFeatures {
  std::string schema_version = kGeometryFeatureSchema;
  std::string mesh_hash;
  std::size_t vertex_count = 0;
  std::size_t triangle_count = 0;
  Vector3 aabb_extent;
  double aabb_diagonal = 0.0;
  double aspect_ratio = 0.0;
  double surface_area = 0.0;
  double absolute_volume_proxy = 0.0;
  double edge_length_min = 0.0;
  double edge_length_mean = 0.0;
  double edge_length_p95 = 0.0;
  double edge_length_max = 0.0;
  double normal_variation = 0.0;
  double geometry_complexity = 0.0;
  bool watertight = false;
  bool valid = false;
};

class GeometryFeatureExtractor {
 public:
  static GeometryFeatures fromMesh(
      const TriangleMesh& mesh,
      const MeshDiagnosticsReport& diagnostics);
};

struct BackendParameterAdvice {
  bool success = false;
  bool used_fallback = false;
  bool out_of_distribution = false;
  std::string source;
  SDFBackendParameters parameters;
  std::vector<std::string> warnings;
};

class BackendParameterAdvisor {
 public:
  virtual ~BackendParameterAdvisor() = default;
  virtual BackendParameterAdvice advise(
      const GeometryFeatures& features,
      const SDFCreationConstraints& constraints) const = 0;
};

class HeuristicBackendParameterAdvisor final : public BackendParameterAdvisor {
 public:
  BackendParameterAdvice advise(
      const GeometryFeatures& features,
      const SDFCreationConstraints& constraints) const override;
};

}  // namespace adasdf
