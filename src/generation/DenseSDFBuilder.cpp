#include "adasdf/generation/DenseSDFBuilder.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>

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

void assignReport(DenseSDFBuildReport* out, const DenseSDFBuildReport& value) {
  if (out != nullptr) {
    *out = value;
  }
}

Vector3 gridPoint(const DenseSDFGrid& grid, int i, int j, int k) {
  return {
      grid.origin.x + static_cast<double>(i) * grid.spacing.x,
      grid.origin.y + static_cast<double>(j) * grid.spacing.y,
      grid.origin.z + static_cast<double>(k) * grid.spacing.z};
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

TriangleMesh maybeCleanup(
    const TriangleMesh& input,
    const DenseSDFBuildOptions& options,
    DenseSDFBuildReport& report) {
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

DenseSDFGrid makeGrid(
    const TriangleMesh& mesh,
    const DenseSDFBuildOptions& options,
    DenseSDFBuildReport& report) {
  DenseSDFGrid grid;
  const MeshAABB box = mesh.aabb();
  const int resolution = options.resolution;
  grid.nx = resolution;
  grid.ny = resolution;
  grid.nz = resolution;
  grid.signed_distance = options.signed_distance;

  Vector3 min_corner = toVector3(box.min);
  Vector3 max_corner = toVector3(box.max);
  const double pad = std::max(0.0, options.padding);
  min_corner = {min_corner.x - pad, min_corner.y - pad, min_corner.z - pad};
  max_corner = {max_corner.x + pad, max_corner.y + pad, max_corner.z + pad};
  const double min_extent = 1.0e-6;
  if (!(max_corner.x > min_corner.x)) {
    min_corner.x -= min_extent;
    max_corner.x += min_extent;
  }
  if (!(max_corner.y > min_corner.y)) {
    min_corner.y -= min_extent;
    max_corner.y += min_extent;
  }
  if (!(max_corner.z > min_corner.z)) {
    min_corner.z -= min_extent;
    max_corner.z += min_extent;
  }

  grid.origin = min_corner;
  grid.spacing = {
      (max_corner.x - min_corner.x) / static_cast<double>(resolution - 1),
      (max_corner.y - min_corner.y) / static_cast<double>(resolution - 1),
      (max_corner.z - min_corner.z) / static_cast<double>(resolution - 1)};
  grid.phi.resize(
      static_cast<std::size_t>(resolution) *
      static_cast<std::size_t>(resolution) *
      static_cast<std::size_t>(resolution));

  bool warned_ambiguous = false;
  for (int k = 0; k < grid.nz; ++k) {
    for (int j = 0; j < grid.ny; ++j) {
      for (int i = 0; i < grid.nx; ++i) {
        const Vector3 p = gridPoint(grid, i, j, k);
        double phi = minTriangleDistance(mesh, p);
        if (!std::isfinite(phi)) {
          phi = 0.0;
        }
        if (options.signed_distance) {
          const MeshSignResult sign = MeshSign::classifyPoint(mesh, p);
          if (sign == MeshSignResult::Inside) {
            phi = -phi;
          } else if (sign == MeshSignResult::OnSurface) {
            phi = 0.0;
          } else if (sign == MeshSignResult::Ambiguous && !warned_ambiguous) {
            warned_ambiguous = true;
            report.warnings.push_back(
                "MeshSign produced ambiguous samples; ambiguous points were "
                "kept as unsigned distances.");
          }
        }
        grid.phi[valueIndex(i, j, k, grid.nx, grid.ny)] = phi;
      }
    }
  }
  return grid;
}

}  // namespace

std::shared_ptr<SDFModel> DenseSDFBuilder::fromMesh(
    const TriangleMesh& mesh,
    const DenseSDFBuildOptions& options,
    DenseSDFBuildReport* report_out) {
  DenseSDFBuildReport report;
  report.signed_distance = options.signed_distance;
  report.padding = options.padding;
  const auto t0 = std::chrono::steady_clock::now();

  if (options.resolution < 2) {
    report.error_message = "DenseSDF resolution must be at least 2.";
    assignReport(report_out, report);
    return nullptr;
  }
  if (mesh.empty()) {
    report.error_message = "DenseSDFBuilder requires a non-empty mesh.";
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
  report.vertex_count = working_mesh.vertexCount();
  report.triangle_count = working_mesh.triangleCount();

  if (options.signed_distance && options.require_watertight_for_signed &&
      !report.diagnostics.watertight) {
    report.error_message =
        "Signed DenseSDF build requires a watertight mesh. Use --unsigned for "
        "open meshes or repair/fill holes before signed build.";
    assignReport(report_out, report);
    return nullptr;
  }

  DenseSDFGrid grid = makeGrid(working_mesh, options, report);
  auto dense = std::make_shared<DenseSDFModel>(std::move(grid));
  if (!dense->isValid()) {
    report.error_message = "DenseSDFModel validation failed after build.";
    assignReport(report_out, report);
    return nullptr;
  }

  const auto t1 = std::chrono::steady_clock::now();
  report.success = true;
  report.nx = dense->grid().nx;
  report.ny = dense->grid().ny;
  report.nz = dense->grid().nz;
  report.memory_bytes = dense->memoryFootprintBytes();
  report.build_time_ms =
      std::chrono::duration<double, std::milli>(t1 - t0).count();
  assignReport(report_out, report);
  return dense;
}

std::shared_ptr<SDFModel> DenseSDFBuilder::fromSTL(
    const std::string& stl_path,
    const DenseSDFBuildOptions& options,
    DenseSDFBuildReport* report) {
  STLReadOptions read_options;
  read_options.vertex_merge_tolerance = options.vertex_merge_tolerance;
  const STLReadResult read = STLReader::read(stl_path, read_options);
  if (!read.success) {
    DenseSDFBuildReport failure;
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
