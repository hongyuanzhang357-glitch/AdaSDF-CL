#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>

namespace {

adasdf::TriangleMesh makeTriangleMesh() {
  adasdf::TriangleMesh mesh;
  mesh.vertices = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
  mesh.triangles = {{0, 1, 2, 0}};
  return mesh;
}

adasdf::AdaptiveBlockSDFModel makeModel() {
  adasdf::AdaptiveSDFBlock block;
  block.block_id = 11;
  block.level = 1;
  block.bounds.min = {-0.1, -0.1, -0.1};
  block.bounds.max = {1.1, 1.1, 0.1};
  block.bounds.valid = true;
  block.origin = block.bounds.min;
  block.spacing = {0.6, 0.6, 0.1};
  block.nx = 3;
  block.ny = 3;
  block.nz = 3;
  block.near_surface = true;
  block.phi.assign(27, 0.001);
  adasdf::AdaptiveSDFBlockSet blocks;
  blocks.global_bounds = block.bounds;
  blocks.blocks.push_back(block);
  return adasdf::AdaptiveBlockSDFModel(blocks);
}

}  // namespace

int main() {
  try {
    const adasdf::TriangleMesh mesh = makeTriangleMesh();
    adasdf::AdaptiveBlockSDFModel model = makeModel();
    adasdf::BlockProvenanceSet provenance;
    adasdf::BlockProvenance block;
    block.block_id = 11;
    block.level = 1;
    block.is_contact_band_block = true;
    block.is_coverage_promoted = true;
    block.has_exact_contact_cells = true;
    block.logical_node_count = 27;
    block.exact_node_count = 27;
    block.promotion_reason = "unit-test";
    provenance.blocks.push_back(block);

    adasdf::NearSurfaceSampleSet samples;
    adasdf::NearSurfaceSample sample;
    sample.surface_sample_id = 0;
    sample.triangle_index = 0;
    sample.point = {0.2, 0.2, 0.001};
    sample.offset = 0.001;
    samples.samples.push_back(sample);
    adasdf::SDFAccuracyAuditOptions options;
    options.near_band = 0.01;
    options.use_provenance = true;
    options.block_provenance = &provenance;
    options.query_source_report = true;
    options.block_source_report = true;
    const adasdf::SDFAccuracyAuditResult result =
        adasdf::SDFAccuracyAudit::run(model, mesh, samples, options);
    if (result.samples.empty() ||
        result.samples.front().query_source != "coverage_promoted_block" ||
        result.samples.front().block_source != "coverage_promoted_block" ||
        result.query_source_bins.empty()) {
      std::cerr << "audit did not classify query source from provenance\n";
      return 1;
    }
    std::cout << "audit query-source explain passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_audit_query_source_explain failed: " << exc.what()
              << "\n";
    return 1;
  }
}
