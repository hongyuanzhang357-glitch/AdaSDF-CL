#pragma once

#include "adasdf/acceleration/BVHNearestTriangleQuery.h"
#include "adasdf/acceleration/BVHRayIntersectionQuery.h"
#include "adasdf/acceleration/BuildAccelerationReport.h"
#include "adasdf/acceleration/TriangleBVHBuilder.h"
#include "adasdf/mesh/TriangleMesh.h"

namespace adasdf {

struct BVHSDFSamplerOptions {
  SDFSamplingAcceleration acceleration = SDFSamplingAcceleration::BruteForce;
  bool signed_distance = true;
  bool fallback_to_bruteforce_sign = true;
  double surface_epsilon = 1e-12;
  TriangleBVHBuildOptions bvh_options;
};

struct BVHSDFSampleResult {
  bool success = false;
  double phi = 0.0;
  bool used_bvh = false;
  bool ambiguous_sign = false;
  bool fallback_sign = false;
  BVHNearestTriangleQueryResult nearest;
  BVHRayIntersectionResult ray;
};

class BVHSDFSampler {
 public:
  bool reset(
      const TriangleMesh& mesh,
      const BVHSDFSamplerOptions& options,
      BuildAccelerationStats* stats = nullptr);

  BVHSDFSampleResult sample(const Vector3& point) const;

  const TriangleBVH& bvh() const { return bvh_; }
  const TriangleMesh* mesh() const { return mesh_; }
  const BVHSDFSamplerOptions& options() const { return options_; }
  bool hasBVH() const { return bvh_.isValid(); }

  static BVHSDFSampleResult sampleBruteForce(
      const TriangleMesh& mesh,
      const Vector3& point,
      bool signed_distance);

  static BVHSDFSampleResult sampleWithBVH(
      const TriangleMesh& mesh,
      const TriangleBVH& bvh,
      const Vector3& point,
      const BVHSDFSamplerOptions& options);

 private:
  const TriangleMesh* mesh_ = nullptr;
  BVHSDFSamplerOptions options_;
  TriangleBVH bvh_;
};

}  // namespace adasdf
