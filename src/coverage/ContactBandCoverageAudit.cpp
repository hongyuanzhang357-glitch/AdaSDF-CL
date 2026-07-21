#include "adasdf/coverage/ContactBandCoverageAudit.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include "adasdf/acceleration/BVHSDFSampler.h"
#include "adasdf/compression/CompressedSDFBlock.h"
#include "adasdf/geometry/AdaptiveBlockSDFModel.h"
#include "adasdf/geometry/CompressedAdaptiveBlockSDFModel.h"

namespace adasdf {
namespace {

int signWithEps(double value, double eps) {
  if (value > eps) {
    return 1;
  }
  if (value < -eps) {
    return -1;
  }
  return 0;
}

struct BlockHit {
  int block_id = -1;
  int level = -1;
  bool near_surface = false;
  bool found = false;
};

BlockHit findBlock(const SDFModel& sdf, const Vector3& point) {
  if (const auto* model = dynamic_cast<const AdaptiveBlockSDFModel*>(&sdf)) {
    const int index = model->findContainingBlock(point);
    if (index < 0) {
      return {};
    }
    const AdaptiveSDFBlock& block =
        model->blockSet().blocks[static_cast<std::size_t>(index)];
    return {block.block_id, block.level, block.near_surface, true};
  }
  if (const auto* model =
          dynamic_cast<const CompressedAdaptiveBlockSDFModel*>(&sdf)) {
    const int index = model->findContainingBlock(point);
    if (index < 0) {
      return {};
    }
    const CompressedSDFBlock& block =
        model->compressedBlockSet().blocks[static_cast<std::size_t>(index)];
    return {block.block_id, block.level, block.near_surface, true};
  }
  return {};
}

bool blockCovered(
    const BlockHit& hit,
    const CoverageAuditOptions& options,
    const BlockProvenanceSet* provenance,
    CoverageMissRecord* record) {
  bool contact = hit.near_surface;
  bool far = hit.found && !hit.near_surface;
  bool exact = hit.near_surface;
  bool predicted = !hit.near_surface;
  if (provenance != nullptr) {
    if (const BlockProvenance* block = provenance->find(hit.block_id)) {
      contact = block->is_contact_band_block || block->is_coverage_promoted;
      far = block->is_far_field_block;
      exact = block->logical_node_count > 0 &&
              block->exact_node_count >= block->logical_node_count;
      predicted = block->predicted_node_count > 0;
    }
  }
  if (record != nullptr) {
    record->hit_contact_block = contact;
    record->hit_far_field_block = far;
    record->hit_predicted_region = predicted && !exact;
  }
  if (options.require_exact_cell_hit) {
    return exact;
  }
  if (options.require_contact_block_hit) {
    return contact;
  }
  return hit.found;
}

std::vector<int> topBlocks(const std::map<int, std::size_t>& counts) {
  std::vector<std::pair<int, std::size_t>> pairs;
  pairs.reserve(counts.size());
  for (const auto& item : counts) {
    pairs.push_back(item);
  }
  std::sort(
      pairs.begin(),
      pairs.end(),
      [](const auto& a, const auto& b) {
        if (a.second != b.second) {
          return a.second > b.second;
        }
        return a.first < b.first;
      });
  std::vector<int> out;
  out.reserve(pairs.size());
  for (const auto& item : pairs) {
    out.push_back(item.first);
  }
  return out;
}

}  // namespace

CoverageAuditResult ContactBandCoverageAudit::run(
    const SDFModel& sdf,
    const TriangleMesh& mesh,
    const CoverageAuditOptions& options,
    const BlockProvenanceSet* provenance) {
  NearSurfaceSampleOptions sample_options;
  sample_options.surface_sample_count =
      static_cast<std::size_t>(std::max(0, options.surface_samples));
  sample_options.offsets.clear();
  for (double offset : options.offsets) {
    if (offset < 0.0 && !options.include_negative_offsets) {
      continue;
    }
    if (offset > 0.0 && !options.include_positive_offsets) {
      continue;
    }
    sample_options.offsets.push_back(offset);
  }
  const NearSurfaceSampleSet samples =
      NearSurfaceSampleGenerator::generate(mesh, sample_options);
  return run(sdf, mesh, samples, options, provenance);
}

CoverageAuditResult ContactBandCoverageAudit::run(
    const SDFModel& sdf,
    const TriangleMesh& mesh,
    const NearSurfaceSampleSet& samples,
    const CoverageAuditOptions& options,
    const BlockProvenanceSet* provenance) {
  CoverageAuditResult result;
  result.total_samples = samples.samples.size();

  BVHSDFSamplerOptions sampler_options;
  sampler_options.acceleration = SDFSamplingAcceleration::BVH;
  sampler_options.signed_distance = true;
  sampler_options.fallback_to_bruteforce_sign = true;
  BVHSDFSampler sampler;
  sampler.reset(mesh, sampler_options);

  for (const NearSurfaceSample& sample : samples.samples) {
    const BVHSDFSampleResult reference = sampler.sample(sample.point);
    if (!reference.success || !std::isfinite(reference.phi)) {
      continue;
    }
    if (std::abs(reference.phi) > options.near_band) {
      continue;
    }
    ++result.near_surface_samples;
    CoverageMissRecord record;
    record.point = sample.point;
    record.reference_phi = reference.phi;
    record.reference_sign = signWithEps(reference.phi, 1.0e-12);
    record.nearest_triangle_id = reference.nearest.triangle_index;

    const BlockHit hit = findBlock(sdf, sample.point);
    record.block_id = hit.block_id;
    record.block_level = hit.level;
    const bool covered = hit.found &&
                         blockCovered(hit, options, provenance, &record);
    if (covered) {
      ++result.covered_samples;
      continue;
    }
    ++result.missed_samples;
    if (options.cluster_by_level) {
      ++result.missed_by_level[record.block_level];
    }
    if (options.cluster_by_block) {
      ++result.missed_by_block_id[record.block_id];
    }
    if (result.representative_misses.size() < 1024) {
      result.representative_misses.push_back(record);
    }
  }

  result.missed_rate =
      result.near_surface_samples == 0
          ? 0.0
          : static_cast<double>(result.missed_samples) /
                static_cast<double>(result.near_surface_samples);
  result.top_missed_block_ids = topBlocks(result.missed_by_block_id);
  result.coverage_passed =
      result.near_surface_samples > 0 && result.missed_samples == 0;
  return result;
}

}  // namespace adasdf
