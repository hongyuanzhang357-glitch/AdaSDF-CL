#include "adasdf/sparse/ContactCandidateReducer.h"

#include <algorithm>
#include <cmath>

namespace adasdf {
namespace {

double distance(const Vector3& a, const Vector3& b) {
  const double dx = a.x - b.x;
  const double dy = a.y - b.y;
  const double dz = a.z - b.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

ContactCandidate makeCandidate(
    const SparseSDFSampleResult& sample,
    double threshold,
    bool compute_normals) {
  ContactCandidate candidate;
  candidate.sample_id = sample.sample_id;
  candidate.point = sample.position;
  candidate.normal = compute_normals ? sample.normal : Vector3{};
  candidate.has_normal = compute_normals && sample.has_normal;
  candidate.phi = sample.phi;
  candidate.effective_phi = sample.effective_phi;
  candidate.penetration_depth = std::max(0.0, threshold - sample.effective_phi);
  candidate.radius = sample.radius;
  candidate.object_id = sample.object_id;
  candidate.link_id = sample.link_id;
  candidate.group_id = sample.group_id;
  candidate.label = sample.label;
  return candidate;
}

}  // namespace

ContactCandidateReductionResult ContactCandidateReducer::reduce(
    const std::vector<SparseSDFSampleResult>& sparse_results,
    const ContactCandidateOptions& options) {
  ContactCandidateReductionResult result;
  result.input_count = sparse_results.size();
  if (options.top_k < 0) {
    result.warnings.push_back("negative top_k treated as zero");
  }
  const int top_k = std::max(0, options.top_k);

  std::vector<SparseSDFSampleResult> filtered;
  for (const SparseSDFSampleResult& sample : sparse_results) {
    if (sample.effective_phi <= options.candidate_threshold) {
      filtered.push_back(sample);
    }
  }
  result.threshold_candidate_count = filtered.size();

  if (options.deterministic_sort) {
    std::sort(
        filtered.begin(),
        filtered.end(),
        [&](const SparseSDFSampleResult& a, const SparseSDFSampleResult& b) {
          if (options.prefer_deeper_penetration &&
              a.effective_phi != b.effective_phi) {
            return a.effective_phi < b.effective_phi;
          }
          if (!options.prefer_deeper_penetration && a.phi != b.phi) {
            return a.phi < b.phi;
          }
          return a.sample_id < b.sample_id;
        });
  }

  for (const SparseSDFSampleResult& sample : filtered) {
    bool suppressed = false;
    if (options.reduction_radius > 0.0) {
      for (const ContactCandidate& kept : result.candidates) {
        if (distance(kept.point, sample.position) <= options.reduction_radius) {
          suppressed = true;
          break;
        }
      }
    }
    if (suppressed) {
      continue;
    }
    ContactCandidate candidate =
        makeCandidate(sample, options.candidate_threshold, options.compute_normals);
    candidate.rank = static_cast<int>(result.candidates.size());
    result.candidates.push_back(candidate);
    if (static_cast<int>(result.candidates.size()) >= top_k) {
      break;
    }
  }

  result.reduced_count = result.candidates.size();
  return result;
}

}  // namespace adasdf
