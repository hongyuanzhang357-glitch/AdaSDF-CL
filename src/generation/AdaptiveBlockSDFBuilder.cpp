#include "adasdf/generation/AdaptiveBlockSDFBuilder.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>

#include "adasdf/generation/AdaptiveBlockPartitioner.h"
#include "adasdf/mesh/MeshCleanup.h"
#include "adasdf/mesh/MeshSign.h"
#include "adasdf/mesh/STLReader.h"
#include "adasdf/mesh/TriangleDistance.h"

namespace adasdf {
namespace {

std::size_t valueIndex(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

bool validIndex(const TriangleMesh& mesh, int index) {
  return index >= 0 && static_cast<std::size_t>(index) < mesh.vertices.size();
}

double minTriangleDistance(const TriangleMesh& mesh, const Vector3& p) {
  double min_dist = std::numeric_limits<double>::infinity();
  for (const MeshTriangle& triangle : mesh.triangles) {
    if (!validIndex(mesh, triangle.v0) || !validIndex(mesh, triangle.v1) ||
        !validIndex(mesh, triangle.v2)) {
      continue;
    }
    const double dist = pointTriangleDistance(
        p,
        toVector3(mesh.vertices[triangle.v0]),
        toVector3(mesh.vertices[triangle.v1]),
        toVector3(mesh.vertices[triangle.v2]));
    if (std::isfinite(dist)) {
      min_dist = std::min(min_dist, dist);
    }
  }
  return min_dist;
}

double signedPhi(
    const TriangleMesh& mesh,
    const Vector3& p,
    bool signed_distance,
    bool* ambiguous) {
  double phi = minTriangleDistance(mesh, p);
  if (!std::isfinite(phi)) {
    return 0.0;
  }
  if (!signed_distance) {
    return phi;
  }
  const MeshSignResult sign = MeshSign::classifyPoint(mesh, p);
  if (sign == MeshSignResult::Inside) {
    phi = -phi;
  } else if (sign == MeshSignResult::OnSurface) {
    phi = 0.0;
  } else if (sign == MeshSignResult::Ambiguous && ambiguous != nullptr) {
    *ambiguous = true;
  }
  return phi;
}

Vector3 gridPoint(const AdaptiveSDFBlock& block, int i, int j, int k) {
  return {
      block.origin.x + static_cast<double>(i) * block.spacing.x,
      block.origin.y + static_cast<double>(j) * block.spacing.y,
      block.origin.z + static_cast<double>(k) * block.spacing.z};
}

void sampleBlocks(
    const TriangleMesh& mesh,
    AdaptiveSDFBlockSet& block_set,
    bool signed_distance,
    AdaptiveBlockSDFBuildReport& report) {
  bool warned_ambiguous = false;
  for (AdaptiveSDFBlock& block : block_set.blocks) {
    block.signed_distance = signed_distance;
    block.phi.resize(
        static_cast<std::size_t>(block.nx) *
        static_cast<std::size_t>(block.ny) *
        static_cast<std::size_t>(block.nz));
    for (int k = 0; k < block.nz; ++k) {
      for (int j = 0; j < block.ny; ++j) {
        for (int i = 0; i < block.nx; ++i) {
          bool ambiguous = false;
          const double phi = signedPhi(
              mesh,
              gridPoint(block, i, j, k),
              signed_distance,
              &ambiguous);
          if (ambiguous && !warned_ambiguous) {
            warned_ambiguous = true;
            report.warnings.push_back(
                "MeshSign produced ambiguous adaptive block samples; ambiguous "
                "points were kept as unsigned distances.");
          }
          block.phi[valueIndex(i, j, k, block.nx, block.ny)] = phi;
        }
      }
    }
  }
}

void assignReport(
    AdaptiveBlockSDFBuildReport* out,
    const AdaptiveBlockSDFBuildReport& value) {
  if (out != nullptr) {
    *out = value;
  }
}

TriangleMesh maybeCleanup(
    const TriangleMesh& input,
    const AdaptiveBlockSDFBuildOptions& options,
    AdaptiveBlockSDFBuildReport& report) {
  if (!options.auto_safe_cleanup) {
    return input;
  }
  MeshCleanupOptions cleanup_options;
  cleanup_options.vertex_merge_tolerance = options.vertex_merge_tolerance;
  cleanup_options.degenerate_area_epsilon = options.degenerate_area_epsilon;
  const MeshCleanupResult cleanup = MeshCleanup::clean(input, cleanup_options);
  if (!cleanup.success) {
    report.warnings.push_back(
        "auto safe cleanup failed and original mesh was kept: " +
        cleanup.error_message);
    return input;
  }
  report.used_cleanup = true;
  for (const std::string& warning : cleanup.stats.warnings) {
    report.warnings.push_back("cleanup: " + warning);
  }
  return cleanup.cleaned_mesh;
}

}  // namespace

std::shared_ptr<SDFModel> AdaptiveBlockSDFBuilder::fromMesh(
    const TriangleMesh& mesh,
    const AdaptiveBlockSDFBuildOptions& options,
    AdaptiveBlockSDFBuildReport* report_out) {
  AdaptiveBlockSDFBuildReport report;
  report.signed_distance = options.signed_distance;
  report.min_octree_level = options.min_octree_level;
  report.max_octree_level = options.max_octree_level;
  report.block_resolution = options.block_resolution;
  const auto t0 = std::chrono::steady_clock::now();

  if (mesh.empty()) {
    report.error_message = "AdaptiveBlockSDFBuilder requires a non-empty mesh.";
    assignReport(report_out, report);
    return nullptr;
  }
  if (options.block_resolution < 2) {
    report.error_message = "Adaptive block resolution must be at least 2.";
    assignReport(report_out, report);
    return nullptr;
  }
  if (options.min_octree_level < 0 ||
      options.max_octree_level < options.min_octree_level) {
    report.error_message = "Adaptive octree level range is invalid.";
    assignReport(report_out, report);
    return nullptr;
  }

  TriangleMesh working_mesh = maybeCleanup(mesh, options, report);

  MeshDiagnosticsOptions diagnostics_options;
  diagnostics_options.duplicate_triangle_tolerance =
      options.vertex_merge_tolerance;
  diagnostics_options.degenerate_area_epsilon = options.degenerate_area_epsilon;
  if (options.run_mesh_diagnostics) {
    report.diagnostics =
        MeshDiagnostics::analyze(working_mesh, diagnostics_options);
  } else {
    report.diagnostics.valid_mesh = !working_mesh.empty();
    report.diagnostics.vertex_count = working_mesh.vertexCount();
    report.diagnostics.triangle_count = working_mesh.triangleCount();
    report.diagnostics.raw_triangle_count = working_mesh.triangleCount();
    report.diagnostics.aabb = working_mesh.aabb();
    report.diagnostics.diagonal_length = working_mesh.diagonalLength();
  }

  MeshReadinessOptions readiness_options;
  readiness_options.require_watertight =
      options.signed_distance && options.require_watertight_for_signed;
  if (options.run_readiness_check) {
    report.readiness =
        MeshReadiness::evaluate(report.diagnostics, readiness_options);
  }
  report.watertight = report.diagnostics.watertight;

  if (options.signed_distance && options.require_watertight_for_signed &&
      !report.diagnostics.watertight) {
    report.error_message =
        "Signed AdaptiveBlockSDF build requires a watertight mesh. Use "
        "--unsigned for open meshes or repair/fill holes before signed build.";
    assignReport(report_out, report);
    return nullptr;
  }

  AdaptiveOctreeBuildOptions octree_options;
  octree_options.min_level = options.min_octree_level;
  octree_options.max_level = options.max_octree_level;
  octree_options.padding = options.padding;
  octree_options.target_near_surface_error =
      options.target_near_surface_error;
  octree_options.surface_band_factor = options.surface_band_factor;
  octree_options.signed_distance = options.signed_distance;
  octree_options.require_watertight_for_signed =
      options.require_watertight_for_signed;

  AdaptiveOctree octree =
      AdaptiveOctreeBuilder::build(working_mesh, octree_options, &report.octree_report);
  if (!report.octree_report.success) {
    report.error_message = report.octree_report.error_message;
    assignReport(report_out, report);
    return nullptr;
  }

  AdaptiveBlockPartitionOptions partition_options;
  partition_options.block_resolution = options.block_resolution;
  partition_options.include_all_leaves = true;
  AdaptiveSDFBlockSet blocks =
      AdaptiveBlockPartitioner::partition(octree, partition_options);
  blocks.signed_distance = options.signed_distance;

  const auto sample0 = std::chrono::steady_clock::now();
  sampleBlocks(working_mesh, blocks, options.signed_distance, report);
  const auto sample1 = std::chrono::steady_clock::now();
  report.sampling_time_ms =
      std::chrono::duration<double, std::milli>(sample1 - sample0).count();

  auto model = std::make_shared<AdaptiveBlockSDFModel>(std::move(blocks));
  if (!model->isValid()) {
    report.error_message =
        "AdaptiveBlockSDFModel validation failed after build.";
    assignReport(report_out, report);
    return nullptr;
  }

  const auto t1 = std::chrono::steady_clock::now();
  report.success = true;
  report.max_octree_level_used = report.octree_report.max_level_used;
  report.octree_node_count = report.octree_report.node_count;
  report.octree_leaf_count = report.octree_report.leaf_count;
  report.block_count = model->blockSet().blockCount();
  report.near_surface_block_count =
      static_cast<std::size_t>(std::count_if(
          model->blockSet().blocks.begin(),
          model->blockSet().blocks.end(),
          [](const AdaptiveSDFBlock& block) { return block.near_surface; }));
  report.memory_bytes = model->memoryFootprintBytes();
  report.build_time_ms =
      std::chrono::duration<double, std::milli>(t1 - t0).count();
  assignReport(report_out, report);
  return model;
}

std::shared_ptr<SDFModel> AdaptiveBlockSDFBuilder::fromSTL(
    const std::string& stl_path,
    const AdaptiveBlockSDFBuildOptions& options,
    AdaptiveBlockSDFBuildReport* report) {
  STLReadOptions read_options;
  read_options.vertex_merge_tolerance = options.vertex_merge_tolerance;
  const STLReadResult read = STLReader::read(stl_path, read_options);
  if (!read.success) {
    AdaptiveBlockSDFBuildReport failure;
    failure.signed_distance = options.signed_distance;
    failure.error_message = "Failed to read STL: " + read.error_message;
    assignReport(report, failure);
    return nullptr;
  }
  auto model = fromMesh(read.mesh, options, report);
  if (report != nullptr) {
    report->diagnostics.raw_triangle_count = read.raw_triangle_count;
  }
  if (model) {
    SDFMetadata metadata = model->metadata();
    metadata.source_path = stl_path;
    model->setMetadata(metadata);
  }
  return model;
}

}  // namespace adasdf
