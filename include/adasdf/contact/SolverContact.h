#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/contact/ContactPatch.h"

namespace adasdf {

struct SolverContact {
  int contact_id = -1;

  int sample_id = -1;
  int patch_id = -1;

  Vector3 point;
  Vector3 normal;

  double penetration_depth = 0.0;
  double phi = 0.0;
  double effective_phi = 0.0;

  int object_id = 0;
  int link_id = 0;
  int group_id = 0;

  std::string label;

  // Stable key for warm-start / temporal matching.
  std::string stable_key;
};

struct SolverContactSet {
  std::vector<SolverContact> contacts;

  std::size_t size() const;
  bool empty() const;
};

class SolverContactBuilder {
 public:
  static SolverContactSet fromCandidates(
      const std::vector<ContactCandidate>& candidates,
      const std::vector<ContactPatch>& patches = {});
};

}  // namespace adasdf
