#include <adasdf/adasdf.h>

#include <iostream>

namespace {

std::size_t gridIndex(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

adasdf::AdaptiveSDFBlock makeBlock() {
  adasdf::AdaptiveSDFBlock block;
  block.nx = 3;
  block.ny = 3;
  block.nz = 3;
  block.spacing = {0.1, 0.1, 0.1};
  block.phi.assign(27, 0.01);
  block.phi[gridIndex(1, 1, 1, 3, 3)] = 0.0;
  return block;
}

adasdf::TriangleMesh makeTriangleMesh() {
  adasdf::TriangleMesh mesh;
  mesh.vertices = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
  mesh.triangles = {{0, 1, 2, 0}};
  return mesh;
}

adasdf::AdaptiveBlockSDFModel makeCoverageModel(bool contact_block) {
  adasdf::AdaptiveSDFBlock block;
  block.block_id = 4;
  block.level = 1;
  block.bounds.min = {-0.1, -0.1, -0.1};
  block.bounds.max = {1.1, 1.1, 0.1};
  block.bounds.valid = true;
  block.origin = block.bounds.min;
  block.spacing = {0.6, 0.6, 0.1};
  block.nx = 3;
  block.ny = 3;
  block.nz = 3;
  block.near_surface = contact_block;
  block.phi.assign(27, 0.0);
  adasdf::AdaptiveSDFBlockSet blocks;
  blocks.global_bounds = block.bounds;
  blocks.blocks.push_back(block);
  return adasdf::AdaptiveBlockSDFModel(blocks);
}

bool runBuildTimeCoverageAuditCheck() {
  const adasdf::TriangleMesh mesh = makeTriangleMesh();
  adasdf::CoverageAuditOptions options;
  options.surface_samples = 4;
  options.offsets = {0.001};
  options.near_band = 0.01;
  adasdf::AdaptiveBlockSDFModel far_model = makeCoverageModel(false);
  adasdf::BlockProvenanceSet far_provenance;
  adasdf::BlockProvenance far_block;
  far_block.block_id = 4;
  far_block.level = 1;
  far_block.is_far_field_block = true;
  far_block.predicted_node_count = 27;
  far_block.logical_node_count = 27;
  far_provenance.blocks.push_back(far_block);
  const adasdf::CoverageAuditResult miss =
      adasdf::ContactBandCoverageAudit::run(
          far_model,
          mesh,
          options,
          &far_provenance);
  if (miss.near_surface_samples == 0 || miss.missed_samples == 0 ||
      miss.top_missed_block_ids.empty() ||
      miss.top_missed_block_ids.front() != 4) {
    std::cerr << "build-time coverage audit should find far-field miss\n";
    return false;
  }

  adasdf::AdaptiveBlockSDFModel contact_model = makeCoverageModel(true);
  adasdf::BlockProvenanceSet contact_provenance;
  adasdf::BlockProvenance contact_block;
  contact_block.block_id = 4;
  contact_block.level = 1;
  contact_block.is_contact_band_block = true;
  contact_block.has_exact_contact_cells = true;
  contact_block.exact_node_count = 27;
  contact_block.logical_node_count = 27;
  contact_provenance.blocks.push_back(contact_block);
  const adasdf::CoverageAuditResult covered =
      adasdf::ContactBandCoverageAudit::run(
          contact_model,
          mesh,
          options,
          &contact_provenance);
  if (covered.missed_samples != 0 || !covered.coverage_passed) {
    std::cerr << "build-time coverage audit should pass contact block\n";
    return false;
  }
  return true;
}

}  // namespace

int main() {
  try {
    adasdf::ContactBandSamplingOptions options;
    options.coverage_audit = true;
    options.coverage_samples_per_axis = 3;
    options.contact_band_width = 5e-4;

    adasdf::ContactBandMask covered;
    covered.nx = 3;
    covered.ny = 3;
    covered.nz = 3;
    covered.contact_band_node.assign(27, 0);
    covered.exact_required.assign(27, 0);
    covered.halo_node.assign(27, 0);
    covered.exact_required[gridIndex(1, 1, 1, 3, 3)] = 1;

    const adasdf::AdaptiveSDFBlock block = makeBlock();
    adasdf::ContactBandQualityMetrics pass =
        adasdf::ContactBandQualityAudit::auditBlock(
            block, block, covered, options);
    if (!pass.coverage_passed ||
        pass.contact_band_coverage_check_count != 1 ||
        pass.missed_contact_band_point_count != 0 ||
        !pass.contact_band_quality_passed) {
      std::cerr << "coverage audit should pass for covered contact point\n";
      return 1;
    }

    covered.exact_required[gridIndex(1, 1, 1, 3, 3)] = 0;
    adasdf::ContactBandQualityMetrics fail =
        adasdf::ContactBandQualityAudit::auditBlock(
            block, block, covered, options);
    if (fail.coverage_passed ||
        fail.missed_contact_band_point_count != 1 ||
        fail.missed_contact_band_cell_count != 1 ||
        fail.contact_band_quality_passed) {
      std::cerr << "coverage audit should fail for missed contact point\n";
      return 1;
    }

    if (!runBuildTimeCoverageAuditCheck()) {
      return 1;
    }

    std::cout << "contact-band coverage audit passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_contact_band_coverage_audit failed: " << exc.what()
              << "\n";
    return 1;
  }
}
