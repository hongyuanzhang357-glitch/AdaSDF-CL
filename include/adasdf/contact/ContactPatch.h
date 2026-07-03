#pragma once

#include <vector>

#include "adasdf/sparse/ContactCandidateReducer.h"

namespace adasdf {

struct ContactPatch {
  int patch_id = -1;

  std::vector<ContactCandidate> members;

  Vector3 centroid;
  Vector3 average_normal;

  double max_penetration_depth = 0.0;
  double mean_penetration_depth = 0.0;

  int representative_sample_id = -1;
};

struct ContactPatchOptions {
  double spatial_radius = 0.02;

  // Cosine threshold. 0.75 means normals within about 41 degrees.
  double normal_cosine_threshold = 0.75;

  int max_patches = 32;

  bool require_normal_consistency = true;
  bool deterministic = true;
};

class ContactPatchBuilder {
 public:
  static ContactPatch fromMembers(
      int patch_id,
      const std::vector<ContactCandidate>& members);
};

}  // namespace adasdf
