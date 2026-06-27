#include "adasdf/generation/SurrogateRecommender.h"

#include <filesystem>

namespace adasdf {

bool SurrogateRecommender::isAvailable() {
  return false;
}

std::vector<SurrogateRecommendation> SurrogateRecommender::recommend(
    const std::string& mesh_path,
    const BuildOptions& user_constraints,
    int top_k) {
  (void)mesh_path;
  (void)top_k;

  // TODO: connect to GeneralSTLSDFSurrogate exported model and metadata.
  // Current C++ API is a placeholder for future surrogate-guided adaptive builder.
  SurrogateRecommendation unavailable;
  unavailable.options = user_constraints;
  unavailable.credible = false;
  unavailable.note =
      "Surrogate recommendation backend is not available in this build.";
  return {unavailable};
}

}  // namespace adasdf
