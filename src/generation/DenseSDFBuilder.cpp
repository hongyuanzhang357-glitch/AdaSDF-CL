#include "adasdf/generation/DenseSDFBuilder.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <limits>

#include "adasdf/acceleration/BVHSDFSampler.h"
#include "adasdf/acceleration/ParallelSampling.h"
#include "adasdf/cache/SDFSampleCache.h"
#include "adasdf/mesh/MeshCleanup.h"
#include "adasdf/mesh/STLReader.h"

namespace adasdf {
namespace {

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

Vector3 gridPointFromIndex(const DenseSDFGrid& grid, std::size_t index) {
  const int i = static_cast<int>(index % static_cast<std::size_t>(grid.nx));
  const int j = static_cast<int>(
      (index / static_cast<std::size_t>(grid.nx)) %
      static_cast<std::size_t>(grid.ny));
  const int k = static_cast<int>(
      index /
      (static_cast<std::size_t>(grid.nx) *
       static_cast<std::size_t>(grid.ny)));
  return gridPoint(grid, i, j, k);
}

double minSpacing(const DenseSDFGrid& grid) {
  double spacing = std::numeric_limits<double>::infinity();
  if (grid.spacing.x > 0.0) {
    spacing = std::min(spacing, grid.spacing.x);
  }
  if (grid.spacing.y > 0.0) {
    spacing = std::min(spacing, grid.spacing.y);
  }
  if (grid.spacing.z > 0.0) {
    spacing = std::min(spacing, grid.spacing.z);
  }
  return std::isfinite(spacing) ? spacing : 1.0;
}

CachedSDFSample toCachedSample(
    const BVHSDFSampleResult& sample,
    bool signed_distance) {
  CachedSDFSample cached;
  cached.phi = sample.success ? sample.phi : 0.0;
  cached.sign_known = signed_distance && sample.success;
  cached.sign = cached.phi < 0.0 ? -1 : (cached.phi > 0.0 ? 1 : 0);
  cached.nearest_triangle_id = sample.nearest.triangle_index;
  cached.finite = std::isfinite(cached.phi);
  return cached;
}

BuildCacheStats buildCacheStatsFromSampleCache(
    const BuildCacheOptions& options,
    const SDFSampleCache& cache) {
  const SDFSampleCacheStats sample = cache.snapshotStats();
  BuildCacheStats stats;
  stats.sample_cache_enabled = options.sample_cache != BuildCacheScope::Off;
  stats.sample_cache_scope = options.sample_cache;
  stats.sample_cache_entries = sample.entry_count;
  stats.sample_cache_hits = sample.hit_count;
  stats.sample_cache_misses = sample.miss_count;
  stats.distance_cache_hits = options.distance_cache ? sample.hit_count : 0;
  stats.distance_cache_misses = options.distance_cache ? sample.miss_count : 0;
  stats.sign_cache_hits = options.sign_cache ? sample.hit_count : 0;
  stats.sign_cache_misses = options.sign_cache ? sample.miss_count : 0;
  stats.distance_queries_saved = sample.distance_query_saved;
  stats.sign_queries_saved = sample.sign_query_saved;
  stats.cache_memory_estimate_bytes =
      sample.entry_count * sizeof(CachedSDFSample);
  stats.cache_lookup_time_ms = sample.lookup_time_ms;
  stats.cache_insert_time_ms = sample.insert_time_ms;
  finalizeBuildCacheStats(&stats);
  return stats;
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

  BuildAccelerationStats stats;
  stats.acceleration = options.acceleration;
  stats.threads_requested = std::max(1, options.threads);

  BVHSDFSamplerOptions sampler_options;
  sampler_options.acceleration = options.acceleration;
  sampler_options.signed_distance = options.signed_distance;
  sampler_options.bvh_options.degenerate_area_epsilon =
      options.degenerate_area_epsilon;
  BVHSDFSampler sampler;
  sampler.reset(mesh, sampler_options, &stats);
  const bool use_bvh =
      options.acceleration == SDFSamplingAcceleration::BVH && sampler.hasBVH();

  const std::size_t total = grid.phi.size();
  SDFSampleCache sample_cache(options.cache_options.cache_max_entries);
  QuantizationOptions quantization;
  quantization.origin = grid.origin;
  quantization.spacing = minSpacing(grid);
  quantization.epsilon = options.cache_options.cache_quantization_epsilon;
  quantization.include_level = false;
  std::atomic<std::size_t> nearest_node_visits{0};
  std::atomic<std::size_t> nearest_triangle_tests{0};
  std::atomic<std::size_t> ray_node_visits{0};
  std::atomic<std::size_t> ray_triangle_tests{0};
  std::atomic<std::size_t> ambiguous_count{0};
  std::atomic<std::size_t> fallback_count{0};
  std::atomic<std::size_t> cache_hits{0};
  ParallelSamplingOptions parallel_options;
  parallel_options.threads = std::max(1, options.threads);
  const ParallelSamplingStats parallel_stats = parallelFor(
      total,
      parallel_options,
      [&](std::size_t index) {
        const Vector3 p = gridPointFromIndex(grid, index);
        if (options.cache_options.sample_cache != BuildCacheScope::Off) {
          CachedSDFSample cached;
          const QuantizedPointKey key =
              QuantizedPointKeyBuilder::fromPoint(p, 0, quantization);
          if (sample_cache.find(key, &cached)) {
            grid.phi[index] = cached.phi;
            cache_hits.fetch_add(1, std::memory_order_relaxed);
            return;
          }
          const BVHSDFSampleResult sample =
              use_bvh
                  ? BVHSDFSampler::sampleWithBVH(
                        mesh,
                        sampler.bvh(),
                        p,
                        sampler_options)
                  : BVHSDFSampler::sampleBruteForce(
                        mesh,
                        p,
                        options.signed_distance);
          grid.phi[index] = sample.success ? sample.phi : 0.0;
          sample_cache.insert(
              key,
              toCachedSample(sample, options.signed_distance));
          nearest_node_visits.fetch_add(
              sample.nearest.node_visits,
              std::memory_order_relaxed);
          nearest_triangle_tests.fetch_add(
              sample.nearest.triangle_tests,
              std::memory_order_relaxed);
          ray_node_visits.fetch_add(
              sample.ray.node_visits,
              std::memory_order_relaxed);
          ray_triangle_tests.fetch_add(
              sample.ray.triangle_tests,
              std::memory_order_relaxed);
          if (sample.ambiguous_sign) {
            ambiguous_count.fetch_add(1, std::memory_order_relaxed);
          }
          if (sample.fallback_sign) {
            fallback_count.fetch_add(1, std::memory_order_relaxed);
          }
          return;
        }
        const BVHSDFSampleResult sample =
            use_bvh
                ? BVHSDFSampler::sampleWithBVH(
                      mesh,
                      sampler.bvh(),
                      p,
                      sampler_options)
                : BVHSDFSampler::sampleBruteForce(
                      mesh,
                      p,
                      options.signed_distance);
        grid.phi[index] = sample.success ? sample.phi : 0.0;
        nearest_node_visits.fetch_add(
            sample.nearest.node_visits,
            std::memory_order_relaxed);
        nearest_triangle_tests.fetch_add(
            sample.nearest.triangle_tests,
            std::memory_order_relaxed);
        ray_node_visits.fetch_add(
            sample.ray.node_visits,
            std::memory_order_relaxed);
        ray_triangle_tests.fetch_add(
            sample.ray.triangle_tests,
            std::memory_order_relaxed);
        if (sample.ambiguous_sign) {
          ambiguous_count.fetch_add(1, std::memory_order_relaxed);
        }
        if (sample.fallback_sign) {
          fallback_count.fetch_add(1, std::memory_order_relaxed);
        }
      });

  stats.sample_count = total - cache_hits.load();
  stats.threads_used = parallel_stats.threads_used;
  stats.sampling_time_ms = parallel_stats.elapsed_ms;
  stats.nearest_node_visits = nearest_node_visits.load();
  stats.nearest_triangle_tests = nearest_triangle_tests.load();
  stats.ray_node_visits = ray_node_visits.load();
  stats.ray_triangle_tests = ray_triangle_tests.load();
  stats.ambiguous_sign_count = ambiguous_count.load();
  stats.fallback_count = fallback_count.load();

  if (options.benchmark_brute_reference &&
      options.acceleration == SDFSamplingAcceleration::BVH) {
    const auto ref0 = std::chrono::steady_clock::now();
    volatile double checksum = 0.0;
    for (std::size_t index = 0; index < total; ++index) {
      const BVHSDFSampleResult sample =
          BVHSDFSampler::sampleBruteForce(
              mesh,
              gridPointFromIndex(grid, index),
              options.signed_distance);
      checksum += sample.phi;
    }
    (void)checksum;
    const auto ref1 = std::chrono::steady_clock::now();
    stats.brute_reference_time_ms =
        std::chrono::duration<double, std::milli>(ref1 - ref0).count();
    if (stats.sampling_time_ms > 0.0) {
      stats.speedup_vs_bruteforce =
          stats.brute_reference_time_ms / stats.sampling_time_ms;
    }
  }

  if (stats.ambiguous_sign_count > 0) {
    report.warnings.push_back(
        "MeshSign produced ambiguous samples; ambiguous points were kept as "
        "unsigned distances or resolved through brute-force fallback.");
  }
  report.used_bvh = use_bvh;
  report.threads_used = stats.threads_used;
  report.acceleration_stats = stats;
  report.cache_stats =
      buildCacheStatsFromSampleCache(options.cache_options, sample_cache);
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
