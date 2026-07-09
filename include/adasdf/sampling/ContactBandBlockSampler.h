#pragma once

#include <string>

#include "adasdf/cache/BuildCacheOptions.h"
#include "adasdf/cache/BuildCacheStats.h"
#include "adasdf/cache/ContactBandMarkerCache.h"
#include "adasdf/acceleration/BVHSDFSampler.h"
#include "adasdf/generation/AdaptiveBlock.h"
#include "adasdf/sampling/ContactBandDiagnostics.h"
#include "adasdf/sampling/ContactBandMarker.h"
#include "adasdf/sampling/ContactBandSamplingPolicy.h"

namespace adasdf {

struct ContactBandBlockSamplingResult {
  bool success = false;
  std::string error_message;

  AdaptiveSDFBlock block;
  ContactBandMask mask;

  std::size_t exact_node_count = 0;
  std::size_t predicted_node_count = 0;
  std::size_t coarse_sample_count = 0;
  std::size_t far_field_node_count = 0;

  bool has_contact_band = false;

  double exact_sampling_time_ms = 0.0;
  double coarse_sampling_time_ms = 0.0;
  double interpolation_time_ms = 0.0;
  double total_time_ms = 0.0;

  ContactBandDiagnostics diagnostics;
  BuildCacheStats cache_stats;
};

class ContactBandBlockSampler {
 public:
  static ContactBandBlockSamplingResult sampleBlock(
      const AABB& block_bounds,
      int block_id,
      int octree_node_id,
      int level,
      int block_resolution,
      bool signed_distance,
      BVHSDFSampler& exact_sampler,
      const TriangleBVH& triangle_bvh,
      const ContactBandSamplingOptions& options,
      const BuildCacheOptions* cache_options = nullptr,
      ContactBandMarkerCache* marker_cache = nullptr);
};

}  // namespace adasdf
