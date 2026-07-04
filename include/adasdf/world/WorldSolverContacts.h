#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/contact/ContactStabilizer.h"
#include "adasdf/contact/SolverContact.h"
#include "adasdf/world/WorldSparseCollision.h"

namespace adasdf {

struct WorldSolverContactOptions {
  WorldSparseCollisionOptions collision_options;
  ContactCandidateOptions candidate_options;
  ContactStabilizationOptions stabilization_options;
  bool stabilize = true;
};

struct WorldSolverContactStats {
  std::size_t raw_candidate_count = 0;
  std::size_t reduced_candidate_count = 0;
  std::size_t patch_count = 0;
  std::size_t solver_contact_count = 0;
  double elapsed_ms = 0.0;
};

struct WorldSolverContactResult {
  bool success = false;
  std::string error_message;
  WorldSparseCollisionResult sparse_result;
  SolverContactSet contacts;
  WorldSolverContactStats stats;
  std::vector<std::string> warnings;
};

class WorldSolverContacts {
 public:
  static WorldSolverContactResult build(
      const CollisionWorld& world,
      const WorldSolverContactOptions& options = WorldSolverContactOptions{});
};

}  // namespace adasdf
