#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/contact/ContactPatch.h"

namespace adasdf {

struct ContactClusterResult {
  std::vector<ContactPatch> patches;

  std::size_t input_candidate_count = 0;
  std::size_t patch_count = 0;

  std::vector<std::string> warnings;
};

class ContactClusterer {
 public:
  static ContactClusterResult cluster(
      const std::vector<ContactCandidate>& candidates,
      const ContactPatchOptions& options = ContactPatchOptions{});
};

}  // namespace adasdf
