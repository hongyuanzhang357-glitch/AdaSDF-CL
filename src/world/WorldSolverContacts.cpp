#include "adasdf/world/WorldSolverContacts.h"

#include <algorithm>
#include <chrono>
#include <sstream>

#include "adasdf/sparse/ContactCandidateReducer.h"

namespace adasdf {
namespace {

std::vector<SparseSDFSampleResult> sparseSamplesForCandidates(
    const WorldSparseCollisionResult& sparse_result) {
  std::vector<SparseSDFSampleResult> samples;
  int stable_sample_id = 0;
  for (const WorldSparsePairResult& pair : sparse_result.pairs) {
    for (const WorldSparseSampleResult& world_sample : pair.violations) {
      SparseSDFSampleResult sample = world_sample.sample;
      sample.position = world_sample.world_position;
      sample.normal = world_sample.world_normal;
      sample.has_normal = world_sample.has_world_normal;
      sample.object_id = world_sample.source_object_id;
      sample.link_id = world_sample.target_object_id;
      sample.group_id = world_sample.pair_id;
      sample.sample_id = stable_sample_id++;
      std::ostringstream label;
      label << "pair=" << world_sample.pair_id
            << ";source=" << world_sample.source_object_id
            << ";target=" << world_sample.target_object_id;
      if (!world_sample.sample.label.empty()) {
        label << ";" << world_sample.sample.label;
      }
      sample.label = label.str();
      samples.push_back(std::move(sample));
    }
  }
  return samples;
}

void renumberContacts(SolverContactSet* contacts) {
  std::sort(
      contacts->contacts.begin(),
      contacts->contacts.end(),
      [](const SolverContact& a, const SolverContact& b) {
        if (a.penetration_depth != b.penetration_depth) {
          return a.penetration_depth > b.penetration_depth;
        }
        if (a.group_id != b.group_id) {
          return a.group_id < b.group_id;
        }
        return a.sample_id < b.sample_id;
      });
  for (std::size_t i = 0; i < contacts->contacts.size(); ++i) {
    SolverContact& contact = contacts->contacts[i];
    contact.contact_id = static_cast<int>(i);
    contact.stable_key =
        std::to_string(contact.group_id) + ":" +
        std::to_string(contact.object_id) + ":" +
        std::to_string(contact.link_id) + ":" +
        std::to_string(contact.patch_id) + ":" +
        std::to_string(contact.sample_id);
  }
}

}  // namespace

WorldSolverContactResult WorldSolverContacts::build(
    const CollisionWorld& world,
    const WorldSolverContactOptions& options) {
  const auto start = std::chrono::steady_clock::now();
  WorldSolverContactResult result;

  WorldSparseCollisionOptions collision_options = options.collision_options;
  collision_options.mode = SparseCollisionMode::CandidateSearch;
  collision_options.early_exit = false;
  collision_options.compute_normals = true;
  collision_options.return_all_violations = true;
  result.sparse_result = WorldSparseCollision::check(world, collision_options);
  if (!result.sparse_result.success) {
    result.error_message = result.sparse_result.error_message;
    return result;
  }

  const std::vector<SparseSDFSampleResult> sparse_samples =
      sparseSamplesForCandidates(result.sparse_result);
  ContactCandidateOptions candidate_options = options.candidate_options;
  candidate_options.compute_normals = true;
  if (candidate_options.candidate_threshold == 0.0) {
    candidate_options.candidate_threshold = collision_options.threshold;
  }
  const ContactCandidateReductionResult reduced =
      ContactCandidateReducer::reduce(sparse_samples, candidate_options);
  result.stats.raw_candidate_count = reduced.input_count;
  result.stats.reduced_candidate_count = reduced.reduced_count;
  result.warnings.insert(
      result.warnings.end(),
      reduced.warnings.begin(),
      reduced.warnings.end());

  std::vector<ContactCandidate> candidates = reduced.candidates;
  std::vector<ContactPatch> patches;
  if (options.stabilize) {
    const ContactStabilizationResult stabilized =
        ContactStabilizer::stabilize(candidates, options.stabilization_options);
    if (!stabilized.success) {
      result.error_message = stabilized.error_message;
      return result;
    }
    candidates = stabilized.stabilized_candidates;
    patches = stabilized.patches;
    result.stats.patch_count = stabilized.stats.patch_count;
    result.warnings.insert(
        result.warnings.end(),
        stabilized.stats.warnings.begin(),
        stabilized.stats.warnings.end());
  }

  result.contacts = SolverContactBuilder::fromCandidates(candidates, patches);
  renumberContacts(&result.contacts);
  result.stats.solver_contact_count = result.contacts.size();
  result.stats.elapsed_ms =
      std::chrono::duration<double, std::milli>(
          std::chrono::steady_clock::now() - start)
          .count();
  result.success = true;
  return result;
}

}  // namespace adasdf
