#include "adasdf/generation/ConstrainedSDFBuilder.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>

#include "adasdf/acceleration/ExactSDFOracle.h"
#include "adasdf/compression/AdaptiveTensorCompressor.h"
#include "adasdf/generation/ConstrainedAdaptiveOctree.h"
#include "adasdf/generation/ConstrainedBlockPartitioner.h"
#include "adasdf/generation/ConstrainedBlockTensorBuilder.h"
#include "adasdf/geometry/ConstrainedSDFModel.h"
#include "adasdf/io/ConstrainedSDFBin.h"
#include "adasdf/mesh/MeshDiagnostics.h"
#include "adasdf/mesh/STLReader.h"
#include "adasdf/recommendation/BackendParameterAdvisor.h"
#include "adasdf/version.h"

namespace adasdf {
namespace {

using Clock = std::chrono::steady_clock;

double elapsedMs(const Clock::time_point& begin) {
  return std::chrono::duration<double, std::milli>(
      Clock::now() - begin).count();
}

TensorCompressionMethod parseMethod(const std::string& value) {
  if (value == "Dense") {
    return TensorCompressionMethod::Dense;
  }
  if (value == "TT") {
    return TensorCompressionMethod::TensorTrain;
  }
  if (value == "HOSVD") {
    return TensorCompressionMethod::HOSVD;
  }
  if (value == "MatrixSVD") {
    return TensorCompressionMethod::MatrixSVD;
  }
  return TensorCompressionMethod::Tucker;
}

std::vector<TensorCompressionMethod> candidateMethods(
    TensorCompressionMethod advised) {
  std::vector<TensorCompressionMethod> methods;
  for (const TensorCompressionMethod method : {
           advised,
           TensorCompressionMethod::TensorTrain,
           TensorCompressionMethod::HOSVD,
           TensorCompressionMethod::MatrixSVD,
           TensorCompressionMethod::Tucker}) {
    if (std::find(methods.begin(), methods.end(), method) == methods.end()) {
      methods.push_back(method);
    }
  }
  return methods;
}

double vectorLength(const Vector3& value) {
  return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}

struct ExactPointKey {
  std::uint64_t x = 0;
  std::uint64_t y = 0;
  std::uint64_t z = 0;

  bool operator==(const ExactPointKey& other) const {
    return x == other.x && y == other.y && z == other.z;
  }

  bool operator<(const ExactPointKey& other) const {
    if (x != other.x) {
      return x < other.x;
    }
    if (y != other.y) {
      return y < other.y;
    }
    return z < other.z;
  }
};

ExactPointKey exactPointKey(const Vector3& point) {
  const auto bits = [](double value) {
    // Preserve the ordered-map behavior that treats -0.0 and +0.0 as equal.
    if (value == 0.0) {
      value = 0.0;
    }
    std::uint64_t out = 0;
    std::memcpy(&out, &value, sizeof(out));
    return out;
  };
  return {bits(point.x), bits(point.y), bits(point.z)};
}

struct SurfaceErrorAuditResult {
  double max_abs_error = 0.0;
  double max_terminal_spacing = 0.0;
  std::uint64_t sample_count = 0;
  bool complete = true;
};

struct SurfaceErrorAuditContext {
  const SDFModel* model = nullptr;
  double error_target = 0.0;
  int max_depth = 0;
  std::uint64_t max_samples = 2000000;
  std::map<ExactPointKey, double> cache;
  SurfaceErrorAuditResult result;
};

double sampleSurfacePoint(SurfaceErrorAuditContext* context, const Vector3& p) {
  const ExactPointKey key = exactPointKey(p);
  const auto found = context->cache.find(key);
  if (found != context->cache.end()) {
    return found->second;
  }
  if (context->result.sample_count >= context->max_samples) {
    context->result.complete = false;
    return std::numeric_limits<double>::infinity();
  }
  const double value = context->model->sampleDistance(p);
  ++context->result.sample_count;
  context->cache.emplace(key, value);
  if (!std::isfinite(value)) {
    context->result.complete = false;
    context->result.max_abs_error = std::numeric_limits<double>::infinity();
  } else {
    context->result.max_abs_error = std::max(
        context->result.max_abs_error, std::abs(value));
  }
  return value;
}

void auditSurfaceTriangle(
    SurfaceErrorAuditContext* context,
    const Vector3& a,
    const Vector3& b,
    const Vector3& c,
    int depth) {
  if (!context->result.complete) {
    return;
  }
  const Vector3 ab = (a + b) * 0.5;
  const Vector3 bc = (b + c) * 0.5;
  const Vector3 ca = (c + a) * 0.5;
  const Vector3 center = (a + b + c) / 3.0;
  const double values[] = {
      sampleSurfacePoint(context, a),
      sampleSurfacePoint(context, b),
      sampleSurfacePoint(context, c),
      sampleSurfacePoint(context, ab),
      sampleSurfacePoint(context, bc),
      sampleSurfacePoint(context, ca),
      sampleSurfacePoint(context, center)};
  if (!context->result.complete) {
    return;
  }
  const auto range = std::minmax_element(std::begin(values), std::end(values));
  const double vertex_mean = (values[0] + values[1] + values[2]) / 3.0;
  const double center_residual = std::abs(values[6] - vertex_mean);
  const double variation = *range.second - *range.first;
  const bool refine = depth < context->max_depth &&
      (depth == 0 ||
       center_residual > context->error_target * 0.0625 ||
       variation > context->error_target * 0.25);
  if (refine) {
    auditSurfaceTriangle(context, a, ab, ca, depth + 1);
    auditSurfaceTriangle(context, ab, b, bc, depth + 1);
    auditSurfaceTriangle(context, ca, bc, c, depth + 1);
    auditSurfaceTriangle(context, ab, bc, ca, depth + 1);
    return;
  }
  context->result.max_terminal_spacing = std::max(
      context->result.max_terminal_spacing,
      std::max({
          vectorLength(b - a),
          vectorLength(c - b),
          vectorLength(a - c)}));
}

SurfaceErrorAuditResult surfaceErrorAudit(
    const TriangleMesh& mesh,
    const SDFModel& model,
    double error_target) {
  SurfaceErrorAuditContext context;
  context.model = &model;
  context.error_target = error_target;
  context.max_depth = mesh.triangleCount() > 100000
      ? 1
      : (mesh.triangleCount() > 10000 ? 2 : 4);
  for (const MeshTriangle& triangle : mesh.triangles) {
    if (triangle.v0 < 0 || triangle.v1 < 0 || triangle.v2 < 0 ||
        static_cast<std::size_t>(triangle.v0) >= mesh.vertices.size() ||
        static_cast<std::size_t>(triangle.v1) >= mesh.vertices.size() ||
        static_cast<std::size_t>(triangle.v2) >= mesh.vertices.size()) {
      context.result.complete = false;
      break;
    }
    auditSurfaceTriangle(
        &context,
        toVector3(mesh.vertices[static_cast<std::size_t>(triangle.v0)]),
        toVector3(mesh.vertices[static_cast<std::size_t>(triangle.v1)]),
        toVector3(mesh.vertices[static_cast<std::size_t>(triangle.v2)]),
        0);
    if (!context.result.complete) {
      break;
    }
  }
  return context.result;
}

struct PredictedZeroSurfaceAuditResult {
  double max_abs_distance = 0.0;
  std::uint64_t sample_count = 0;
  bool complete = true;
};

Vector3 tensorPoint(
    const ConstrainedSDFBinBlockRecord& block,
    int ix,
    int iy,
    int iz) {
  const auto coordinate = [](double lo, double hi, int index, int count) {
    return count > 1
        ? lo + (hi - lo) * static_cast<double>(index) /
            static_cast<double>(count - 1)
        : lo;
  };
  return {
      coordinate(block.bounds.min.x, block.bounds.max.x, ix, block.nx),
      coordinate(block.bounds.min.y, block.bounds.max.y, iy, block.ny),
      coordinate(block.bounds.min.z, block.bounds.max.z, iz, block.nz)};
}

void auditZeroEdge(
    const ConstrainedSDFBinBlockRecord& block,
    const std::vector<double>& phi,
    int ax,
    int ay,
    int az,
    int bx,
    int by,
    int bz,
    ExactSDFOracle* oracle,
    double error_target,
    std::set<ExactPointKey>* visited,
    PredictedZeroSurfaceAuditResult* result) {
  if (!result->complete) {
    return;
  }
  const auto index = [&](int x, int y, int z) {
    return static_cast<std::size_t>(x) +
        static_cast<std::size_t>(block.nx) *
            (static_cast<std::size_t>(y) +
             static_cast<std::size_t>(block.ny) *
                 static_cast<std::size_t>(z));
  };
  const double phi_a = phi[index(ax, ay, az)];
  const double phi_b = phi[index(bx, by, bz)];
  if (!std::isfinite(phi_a) || !std::isfinite(phi_b)) {
    result->complete = false;
    result->max_abs_distance = std::numeric_limits<double>::infinity();
    return;
  }
  if ((phi_a < 0.0 && phi_b < 0.0) ||
      (phi_a > 0.0 && phi_b > 0.0)) {
    return;
  }
  const Vector3 a = tensorPoint(block, ax, ay, az);
  const Vector3 b = tensorPoint(block, bx, by, bz);
  const double denominator = std::abs(phi_a) + std::abs(phi_b);
  const double t = denominator > 0.0 ? std::abs(phi_a) / denominator : 0.5;
  const Vector3 point = a + (b - a) * t;
  const ExactPointKey key = exactPointKey(point);
  if (!visited->insert(key).second) {
    return;
  }
  constexpr std::uint64_t kMaxZeroCrossingSamples = 2000000;
  if (result->sample_count >= kMaxZeroCrossingSamples) {
    result->complete = false;
    result->max_abs_distance = std::numeric_limits<double>::infinity();
    return;
  }
  const ExactSDFOracleSample sample =
      oracle->sampleUnsignedDistance(point, error_target);
  ++result->sample_count;
  if (!sample.success || !std::isfinite(sample.phi)) {
    result->complete = false;
    result->max_abs_distance = std::numeric_limits<double>::infinity();
    return;
  }
  result->max_abs_distance = std::max(
      result->max_abs_distance, std::abs(sample.phi));
}

PredictedZeroSurfaceAuditResult predictedZeroSurfaceAudit(
    const ConstrainedSDFBinReader& reader,
    ExactSDFOracle* oracle,
    double error_target) {
  PredictedZeroSurfaceAuditResult result;
  std::set<ExactPointKey> visited;
  for (std::size_t block_index = 0;
       block_index < reader.blocks().size();
       ++block_index) {
    const ConstrainedSDFBinBlockRecord& block = reader.blocks()[block_index];
    CompressedTensor3D tensor;
    std::string error;
    if (!reader.loadBlock(block_index, &tensor, &error)) {
      result.complete = false;
      result.max_abs_distance = std::numeric_limits<double>::infinity();
      break;
    }
    const std::vector<double> phi = AdaptiveTensorCompressor::decode(tensor);
    const std::size_t expected_size =
        static_cast<std::size_t>(tensor.nx) *
        static_cast<std::size_t>(tensor.ny) *
        static_cast<std::size_t>(tensor.nz);
    if (phi.size() != expected_size) {
      result.complete = false;
      result.max_abs_distance = std::numeric_limits<double>::infinity();
      break;
    }
    for (int iz = 0; iz < tensor.nz && result.complete; ++iz) {
      for (int iy = 0; iy < tensor.ny && result.complete; ++iy) {
        for (int ix = 0; ix + 1 < tensor.nx && result.complete; ++ix) {
          auditZeroEdge(
              block, phi, ix, iy, iz, ix + 1, iy, iz,
              oracle, error_target, &visited, &result);
        }
      }
    }
    for (int iz = 0; iz < tensor.nz && result.complete; ++iz) {
      for (int iy = 0; iy + 1 < tensor.ny && result.complete; ++iy) {
        for (int ix = 0; ix < tensor.nx && result.complete; ++ix) {
          auditZeroEdge(
              block, phi, ix, iy, iz, ix, iy + 1, iz,
              oracle, error_target, &visited, &result);
        }
      }
    }
    for (int iz = 0; iz + 1 < tensor.nz && result.complete; ++iz) {
      for (int iy = 0; iy < tensor.ny && result.complete; ++iy) {
        for (int ix = 0; ix < tensor.nx && result.complete; ++ix) {
          auditZeroEdge(
              block, phi, ix, iy, iz, ix, iy, iz + 1,
              oracle, error_target, &visited, &result);
        }
      }
    }
  }
  if (result.sample_count == 0) {
    result.complete = false;
    result.max_abs_distance = std::numeric_limits<double>::infinity();
  }
  return result;
}

struct ZeroSurfaceErrorAuditResult {
  SurfaceErrorAuditResult mesh_surface;
  PredictedZeroSurfaceAuditResult predicted_surface;

  bool complete() const {
    return mesh_surface.complete && predicted_surface.complete;
  }

  double maxAbsError() const {
    return std::max(
        mesh_surface.max_abs_error, predicted_surface.max_abs_distance);
  }
};

ZeroSurfaceErrorAuditResult zeroSurfaceErrorAudit(
    const TriangleMesh& mesh,
    const ConstrainedSDFModel& model,
    ExactSDFOracle* oracle,
    double error_target) {
  ZeroSurfaceErrorAuditResult result;
  result.mesh_surface = surfaceErrorAudit(mesh, model, error_target);
  result.predicted_surface =
      predictedZeroSurfaceAudit(model.reader(), oracle, error_target);
  return result;
}

std::filesystem::path temporaryPath(
    const std::filesystem::path& output,
    int attempt) {
  return std::filesystem::path(
      output.string() + ".attempt" + std::to_string(attempt) + ".tmp");
}

void removeQuietly(const std::filesystem::path& path) {
  std::error_code error;
  std::filesystem::remove(path, error);
}

std::size_t counterDelta(std::size_t after, std::size_t before) {
  return after >= before ? after - before : 0;
}

double timingDelta(double after, double before) {
  return after >= before ? after - before : 0.0;
}

void captureOracleDelta(
    SDFCreationAttempt* attempt,
    const ExactSDFOracleStats& before,
    const ExactSDFOracleStats& after) {
  attempt->exact_query_requests = counterDelta(
      after.exact_query_requests, before.exact_query_requests);
  attempt->unique_exact_queries = counterDelta(
      after.unique_exact_queries, before.unique_exact_queries);
  attempt->exact_query_cache_hits = counterDelta(
      after.cache_hits, before.cache_hits);
  attempt->bvh_node_visits = counterDelta(
      after.totalBVHNodeVisits(), before.totalBVHNodeVisits());
  attempt->triangle_tests = counterDelta(
      after.totalTriangleTests(), before.totalTriangleTests());
  attempt->exact_phi_time_ms = timingDelta(
      after.exact_query_time_ms, before.exact_query_time_ms);
}

struct FeasibleCandidate {
  bool available = false;
  std::filesystem::path path;
  SDFCreationMeasurements measurements;
  SDFCreationAttempt attempt;
};

bool candidateIsBetter(
    const SDFCreationAttempt& candidate,
    const SDFCreationAttempt& incumbent) {
  return std::tie(
             candidate.measurements.sdf_file_bytes,
             candidate.total_time_ms,
             candidate.query_time_ms,
             candidate.attempt_index) <
      std::tie(
             incumbent.measurements.sdf_file_bytes,
             incumbent.total_time_ms,
             incumbent.query_time_ms,
             incumbent.attempt_index);
}

SDFCreationResult invalidResult(
    const SDFCreationConstraints& constraints,
    SDFCreationStatus status,
    const std::string& message) {
  SDFCreationResult result;
  result.report.requested = constraints;
  result.report.status = status;
  result.report.message = message;
  return result;
}

SDFCreationReport minimumMemoryInfeasible(
    const SDFCreationConstraints& constraints) {
  SDFCreationMeasurements measurements;
  measurements.max_decoded_block_bytes = 128;
  SDFCreationAttempt attempt;
  attempt.attempt_index = 0;
  attempt.measurements = measurements;
  attempt.failed_constraints = {SDFHardConstraint::MaxDecodedBlockBytes};
  attempt.message = "minimum 2x2x2 block output and decode workspace";
  return evaluateSDFCreationConstraints(constraints, measurements, {attempt});
}

void attachFeatureEvidence(
    SDFCreationReport* report,
    const GeometryFeatures& features,
    const BackendParameterAdvice& advice) {
  report->library_version = versionString();
  report->mesh_hash = features.mesh_hash;
  report->feature_schema = features.schema_version;
  report->advisor_source = advice.source;
  report->advisor_used_fallback = advice.used_fallback;
  report->advisor_out_of_distribution = advice.out_of_distribution;
  report->advisor_warnings = advice.warnings;
  report->geometry_features = {
      {"vertex_count", static_cast<double>(features.vertex_count)},
      {"triangle_count", static_cast<double>(features.triangle_count)},
      {"aabb_extent_x", features.aabb_extent.x},
      {"aabb_extent_y", features.aabb_extent.y},
      {"aabb_extent_z", features.aabb_extent.z},
      {"aabb_diagonal", features.aabb_diagonal},
      {"aspect_ratio", features.aspect_ratio},
      {"surface_area", features.surface_area},
      {"absolute_volume_proxy", features.absolute_volume_proxy},
      {"edge_length_min", features.edge_length_min},
      {"edge_length_mean", features.edge_length_mean},
      {"edge_length_p95", features.edge_length_p95},
      {"edge_length_max", features.edge_length_max},
      {"normal_variation", features.normal_variation},
      {"geometry_complexity", features.geometry_complexity},
      {"watertight", features.watertight ? 1.0 : 0.0}};
}

BackendParameterAdvice selectBackendAdvice(
    const GeometryFeatures& features,
    const SDFCreationConstraints& constraints) {
  HeuristicBackendParameterAdvisor heuristic;
  return heuristic.advise(features, constraints);
}

}  // namespace

SDFCreationResult ConstrainedSDFBuilder::fromSTL(
    const std::filesystem::path& stl_path,
    const std::filesystem::path& output_path,
    const SDFCreationConstraints& constraints) {
  std::string constraint_error;
  if (!validateSDFCreationConstraints(constraints, &constraint_error)) {
    return invalidResult(
        constraints, SDFCreationStatus::InvalidInput, constraint_error);
  }
  const STLReadResult read = STLReader::read(stl_path.string());
  if (!read.success) {
    return invalidResult(
        constraints,
        SDFCreationStatus::InvalidInput,
        "failed to read STL: " + read.error_message);
  }
  return fromMesh(read.mesh, output_path, constraints);
}

namespace {

SDFCreationResult buildConstrainedSDF(
    const TriangleMesh& mesh,
    const std::filesystem::path& output_path,
    const SDFCreationConstraints& constraints,
    const GeometryFeatures& features,
    const BackendParameterAdvice& advice,
    bool explicit_parameters_only) {

  ExactSDFOracleOptions oracle_options;
  oracle_options.bvh_leaf_size = advice.parameters.bvh_leaf_size;
  ExactSDFOracle oracle;
  std::string oracle_error;
  const Clock::time_point bvh_begin = Clock::now();
  if (!oracle.reset(mesh, oracle_options, &oracle_error)) {
    return invalidResult(
        constraints, SDFCreationStatus::BuildFailed, oracle_error);
  }
  const double bvh_time_ms = elapsedMs(bvh_begin);
  const int base_depth = advice.parameters.octree_max_depth;
  const TensorCompressionMethod advised_method =
      parseMethod(advice.parameters.compression_method);
  const std::vector<TensorCompressionMethod> base_methods =
      explicit_parameters_only
      ? std::vector<TensorCompressionMethod>{advised_method}
      : candidateMethods(advised_method);
  std::vector<SDFCreationAttempt> attempts;
  SDFCreationMeasurements last_measurements;
  FeasibleCandidate best_candidate;

  constexpr int kRepairAttempts = 2;
  const int max_attempts = static_cast<int>(base_methods.size()) +
      (explicit_parameters_only ? 0 : kRepairAttempts);
  for (int attempt_index = 0; attempt_index < max_attempts; ++attempt_index) {
    const bool repair_attempt =
        attempt_index >= static_cast<int>(base_methods.size());
    if (repair_attempt && best_candidate.available) {
      break;
    }
    const int repair_index = repair_attempt
        ? attempt_index - static_cast<int>(base_methods.size())
        : -1;
    const Clock::time_point attempt_begin = Clock::now();
    const ExactSDFOracleStats oracle_stats_begin = oracle.stats();
    SDFCreationAttempt attempt;
    attempt.attempt_index = attempt_index;
    attempt.bvh_build_time_ms = attempt_index == 0 ? bvh_time_ms : 0.0;
    const int depth = repair_attempt
        ? std::min(8, base_depth + repair_index + 1)
        : base_depth;
    const TensorCompressionMethod candidate_method = repair_attempt
        ? base_methods[static_cast<std::size_t>(
              repair_index % static_cast<int>(base_methods.size()))]
        : base_methods[static_cast<std::size_t>(attempt_index)];
    const double repair_scale = repair_attempt
        ? std::ldexp(0.25, -(repair_index + 1))
        : 0.25;
    const int compression_max_rank = std::min(
        64,
        std::max(1, advice.parameters.compression_max_rank) +
            std::max(0, repair_index + 1) * 4);
    attempt.parameters = advice.parameters;
    if (!explicit_parameters_only) {
      attempt.parameters.octree_max_depth = depth;
      attempt.parameters.octree_interpolation_residual_scale =
          std::min(
              advice.parameters.octree_interpolation_residual_scale,
              repair_scale);
      attempt.parameters.block_target_nodes = repair_attempt
          ? std::max(
                advice.parameters.block_min_nodes,
                advice.parameters.block_target_nodes >> (repair_index + 1))
          : advice.parameters.block_target_nodes;
      attempt.parameters.compression_method = toString(candidate_method);
      attempt.parameters.compression_abs_tolerance =
          constraints.max_zero_surface_abs_error * repair_scale;
      attempt.parameters.compression_max_rank = compression_max_rank;
    }
    const TensorCompressionMethod method =
        parseMethod(attempt.parameters.compression_method);

    ConstrainedAdaptiveOctreeOptions tree_options;
    tree_options.min_depth = attempt.parameters.octree_min_depth;
    tree_options.max_depth = attempt.parameters.octree_max_depth;
    tree_options.narrow_band_distance =
        constraints.max_zero_surface_abs_error *
        attempt.parameters.octree_narrow_band_scale;
    tree_options.max_center_interpolation_residual =
        constraints.max_zero_surface_abs_error *
        attempt.parameters.octree_interpolation_residual_scale;
    tree_options.max_overlapping_triangles =
        attempt.parameters.octree_max_overlapping_triangles;
    tree_options.max_normal_complexity =
        attempt.parameters.octree_max_normal_complexity;
    const Clock::time_point tree_begin = Clock::now();
    ConstrainedAdaptiveOctreeStats tree_stats;
    ConstrainedAdaptiveOctree tree =
        ConstrainedAdaptiveOctreeBuilder::build(
            mesh, &oracle, tree_options, &tree_stats);
    attempt.octree_build_time_ms = elapsedMs(tree_begin);
    if (!tree_stats.success) {
      attempt.message = tree_stats.error_message;
      attempt.total_time_ms = elapsedMs(attempt_begin);
      captureOracleDelta(&attempt, oracle_stats_begin, oracle.stats());
      attempts.push_back(attempt);
      continue;
    }
    attempt.exact_node_count = tree_stats.unique_exact_node_count;
    attempt.bvh_node_visits = tree_stats.bvh_node_visits;
    attempt.triangle_tests = tree_stats.triangle_tests;
    attempt.octree_node_count = tree_stats.node_count;
    attempt.octree_leaf_count = tree_stats.leaf_count;
    attempt.uniform_grid_node_count = tree_stats.uniform_finest_grid_node_count;

    ConstrainedBlockPartitionOptions block_options;
    block_options.min_leaf_cells_per_block = static_cast<std::size_t>(
        attempt.parameters.block_min_nodes);
    block_options.target_leaf_cells_per_block =
        static_cast<std::size_t>(attempt.parameters.block_target_nodes);
    block_options.max_leaf_cells_per_block = static_cast<std::size_t>(
        attempt.parameters.block_max_nodes);
    block_options.min_tensor_dimension =
        attempt.parameters.block_min_tensor_dimension;
    block_options.max_tensor_dimension =
        attempt.parameters.block_max_tensor_dimension;
    block_options.halo_nodes = attempt.parameters.block_halo;
    block_options.max_decoded_block_bytes =
        constraints.max_decoded_block_bytes;
    const Clock::time_point block_begin = Clock::now();
    ConstrainedBlockPartitionStats partition_stats;
    const std::vector<ConstrainedSDFBlockPlan> block_plans =
        ConstrainedBlockPartitioner::partition(
            tree, block_options, &partition_stats);
    attempt.block_partition_time_ms = elapsedMs(block_begin);
    if (!partition_stats.success) {
      attempt.message = partition_stats.error_message;
      attempt.failed_constraints = {
          SDFHardConstraint::MaxDecodedBlockBytes};
      attempt.measurements.max_decoded_block_bytes = 128;
      attempt.total_time_ms = elapsedMs(attempt_begin);
      captureOracleDelta(&attempt, oracle_stats_begin, oracle.stats());
      attempts.push_back(attempt);
      continue;
    }
    attempt.block_count = block_plans.size();

    ConstrainedSDFAsset asset;
    asset.octree = std::move(tree);
    asset.zero_surface_error_is_strict_bound = false;
    asset.blocks.reserve(block_plans.size());
    const Clock::time_point tensor_begin = Clock::now();
    bool tensor_failed = false;
    for (const ConstrainedSDFBlockPlan& block_plan : block_plans) {
      ConstrainedBlockTensorStats tensor_stats;
      ConstrainedBlockTensor tensor = ConstrainedBlockTensorBuilder::build(
          asset.octree, block_plan, &oracle, {}, &tensor_stats);
      if (!tensor_stats.success) {
        attempt.message = tensor_stats.error_message;
        tensor_failed = true;
        break;
      }
      ConstrainedBlockTensorStats promotion_stats;
      if (!ConstrainedBlockTensorBuilder::auditAndPromote(
              asset.octree,
              &tensor,
              &oracle,
              attempt.parameters.compression_abs_tolerance,
              &promotion_stats)) {
        attempt.message = promotion_stats.error_message;
        tensor_failed = true;
        break;
      }
      attempt.exact_node_count += tensor_stats.exact_bvh_node_count;
      attempt.interpolation_time_ms += tensor_stats.interpolation_time_ms;
      attempt.interpolated_node_count +=
          promotion_stats.coarse_interpolated_node_count;
      attempt.promoted_node_count += promotion_stats.promoted_exact_node_count;

      AdaptiveTensorCompressionOptions compression_options;
      compression_options.method = method;
      compression_options.min_rank = 1;
      compression_options.max_rank =
          attempt.parameters.compression_max_rank;
      compression_options.max_abs_error =
          attempt.parameters.compression_abs_tolerance;
      compression_options.near_zero_band =
          constraints.max_zero_surface_abs_error * 2.0;
      compression_options.near_zero_max_abs_error =
          attempt.parameters.compression_abs_tolerance * 0.5;
      AdaptiveTensorCompressionReport compression_report;
      const Clock::time_point compression_begin = Clock::now();
      CompressedTensor3D compressed = AdaptiveTensorCompressor::compress(
          tensor.phi,
          block_plan.tensor_nx,
          block_plan.tensor_ny,
          block_plan.tensor_nz,
          compression_options,
          &compression_report);
      attempt.compression_time_ms += elapsedMs(compression_begin);
      if (!compression_report.success) {
        attempt.message = compression_report.error_message;
        tensor_failed = true;
        break;
      }
      attempt.original_tensor_bytes += compressed.original_bytes;
      attempt.compressed_tensor_bytes += compressed.compressed_bytes;
      if (compressed.method == TensorCompressionMethod::Dense) {
        ++attempt.dense_fallback_block_count;
      } else {
        ++attempt.compressed_block_count;
      }
      const int rank_min = std::min({
          compressed.rank_x, compressed.rank_y, compressed.rank_z});
      const int rank_max = std::max({
          compressed.rank_x, compressed.rank_y, compressed.rank_z});
      attempt.rank_min = attempt.rank_min == 0
          ? rank_min
          : std::min(attempt.rank_min, rank_min);
      attempt.rank_max = std::max(attempt.rank_max, rank_max);
      ConstrainedSDFAssetBlock block;
      block.plan = block_plan;
      block.tensor = std::move(compressed);
      asset.blocks.push_back(std::move(block));
    }
    attempt.tensor_build_time_ms = elapsedMs(tensor_begin) -
        attempt.compression_time_ms;
    if (tensor_failed) {
      attempt.total_time_ms = elapsedMs(attempt_begin);
      captureOracleDelta(&attempt, oracle_stats_begin, oracle.stats());
      attempts.push_back(attempt);
      continue;
    }

    const std::filesystem::path temp =
        temporaryPath(output_path, attempt_index);
    removeQuietly(temp);
    const Clock::time_point serialization_begin = Clock::now();
    ConstrainedSDFBinReport write_report;
    if (!ConstrainedSDFBin::write(temp, asset, &write_report)) {
      attempt.message = write_report.error_message;
      attempt.serialization_time_ms = elapsedMs(serialization_begin);
      attempt.total_time_ms = elapsedMs(attempt_begin);
      captureOracleDelta(&attempt, oracle_stats_begin, oracle.stats());
      attempts.push_back(attempt);
      removeQuietly(temp);
      continue;
    }
    attempt.serialization_time_ms += elapsedMs(serialization_begin);
    std::shared_ptr<ConstrainedSDFModel> candidate;
    ZeroSurfaceErrorAuditResult surface_audit;
    try {
      const Clock::time_point reload_begin = Clock::now();
      candidate = ConstrainedSDFModel::load(temp);
      attempt.reload_validation_time_ms += elapsedMs(reload_begin);
      const Clock::time_point zero_surface_begin = Clock::now();
      surface_audit = zeroSurfaceErrorAudit(
          mesh, *candidate, &oracle, constraints.max_zero_surface_abs_error);
      attempt.zero_surface_validation_time_ms +=
          elapsedMs(zero_surface_begin);
      asset.max_zero_surface_abs_error = surface_audit.maxAbsError();
      const Clock::time_point rewrite_begin = Clock::now();
      if (!ConstrainedSDFBin::write(temp, asset, &write_report)) {
        throw std::runtime_error(write_report.error_message);
      }
      attempt.serialization_time_ms += elapsedMs(rewrite_begin);
      const Clock::time_point final_reload_begin = Clock::now();
      candidate = ConstrainedSDFModel::load(temp);
      attempt.reload_validation_time_ms += elapsedMs(final_reload_begin);
      const Clock::time_point final_zero_surface_begin = Clock::now();
      surface_audit = zeroSurfaceErrorAudit(
          mesh, *candidate, &oracle, constraints.max_zero_surface_abs_error);
      attempt.zero_surface_validation_time_ms +=
          elapsedMs(final_zero_surface_begin);
    } catch (const std::exception& exc) {
      attempt.message = exc.what();
      attempt.validation_time_ms = attempt.reload_validation_time_ms +
          attempt.zero_surface_validation_time_ms;
      attempt.total_time_ms = elapsedMs(attempt_begin);
      captureOracleDelta(&attempt, oracle_stats_begin, oracle.stats());
      attempts.push_back(attempt);
      removeQuietly(temp);
      continue;
    }
    attempt.validation_time_ms = attempt.reload_validation_time_ms +
        attempt.zero_surface_validation_time_ms;
    const Clock::time_point query_begin = Clock::now();
    const AABB query_bounds = candidate->boundingBox();
    constexpr int kQueryAxisSamples = 10;
    constexpr std::uint64_t kQuerySampleCount =
        kQueryAxisSamples * kQueryAxisSamples * kQueryAxisSamples;
    volatile double query_sink = 0.0;
    for (int iz = 0; iz < kQueryAxisSamples; ++iz) {
      for (int iy = 0; iy < kQueryAxisSamples; ++iy) {
        for (int ix = 0; ix < kQueryAxisSamples; ++ix) {
          const Vector3 fraction{
              (static_cast<double>(ix) + 0.5) / kQueryAxisSamples,
              (static_cast<double>(iy) + 0.5) / kQueryAxisSamples,
              (static_cast<double>(iz) + 0.5) / kQueryAxisSamples};
          const Vector3 point{
              query_bounds.min.x +
                  fraction.x * (query_bounds.max.x - query_bounds.min.x),
              query_bounds.min.y +
                  fraction.y * (query_bounds.max.y - query_bounds.min.y),
              query_bounds.min.z +
                  fraction.z * (query_bounds.max.z - query_bounds.min.z)};
          query_sink += candidate->sampleDistance(point);
        }
      }
    }
    (void)query_sink;
    attempt.query_time_ms = elapsedMs(query_begin);
    attempt.query_sample_count = kQuerySampleCount;
    attempt.query_ns_per_sample =
        attempt.query_time_ms * 1.0e6 /
        static_cast<double>(kQuerySampleCount);
    SDFCreationMeasurements measurements;
    measurements.sdf_file_bytes = write_report.file_bytes;
    measurements.max_decoded_block_bytes = 0;
    for (const ConstrainedSDFBinBlockRecord& block : candidate->reader().blocks()) {
      measurements.max_decoded_block_bytes = std::max(
          measurements.max_decoded_block_bytes,
          block.decoded_peak_bytes);
    }
    measurements.max_zero_surface_abs_error = surface_audit.maxAbsError();
    measurements.serialized_round_trip_verified = true;
    measurements.zero_surface_error_is_strict_bound = false;
    measurements.zero_surface_validation_complete = surface_audit.complete();
    measurements.zero_surface_validation_sample_count =
        surface_audit.mesh_surface.sample_count;
    measurements.zero_surface_validation_max_spacing =
        surface_audit.mesh_surface.max_terminal_spacing;
    measurements.mesh_to_sdf_max_abs_phi =
        surface_audit.mesh_surface.max_abs_error;
    measurements.sdf_to_mesh_max_abs_distance =
        surface_audit.predicted_surface.max_abs_distance;
    measurements.zero_crossing_validation_sample_count =
        surface_audit.predicted_surface.sample_count;
    last_measurements = measurements;
    const SDFCreationReport evaluated =
        evaluateSDFCreationConstraints(constraints, measurements);
    attempt.measurements = measurements;
    attempt.failed_constraints = evaluated.failed_constraints;
    attempt.message = evaluated.message;
    attempt.total_time_ms = elapsedMs(attempt_begin);
    captureOracleDelta(&attempt, oracle_stats_begin, oracle.stats());
    attempts.push_back(attempt);

    if (!evaluated.feasible()) {
      removeQuietly(temp);
      continue;
    }
    if (!best_candidate.available ||
        candidateIsBetter(attempt, best_candidate.attempt)) {
      if (best_candidate.available) {
        removeQuietly(best_candidate.path);
      }
      best_candidate.available = true;
      best_candidate.path = temp;
      best_candidate.measurements = measurements;
      best_candidate.attempt = attempt;
    } else {
      removeQuietly(temp);
    }
  }

  if (best_candidate.available) {
    try {
      if (!output_path.parent_path().empty()) {
        std::filesystem::create_directories(output_path.parent_path());
      }
      removeQuietly(output_path);
      std::filesystem::rename(best_candidate.path, output_path);
      SDFCreationResult result;
      result.model = ConstrainedSDFModel::load(output_path);
      result.report = evaluateSDFCreationConstraints(
          constraints, best_candidate.measurements, attempts);
      result.report.selected_attempt_index =
          best_candidate.attempt.attempt_index;
      result.report.has_selected_backend_parameters = true;
      result.report.selected_backend_parameters =
          best_candidate.attempt.parameters;
      attachFeatureEvidence(&result.report, features, advice);
      result.report.message = explicit_parameters_only
          ? "explicit backend parameters satisfied all hard constraints "
            "after serialized reload"
          : "all hard constraints satisfied after serialized reload; "
            "selected by file size, build time, and query latency";
      return result;
    } catch (const std::exception& exc) {
      removeQuietly(best_candidate.path);
      SDFCreationResult result = invalidResult(
          constraints, SDFCreationStatus::BuildFailed, exc.what());
      result.report.attempts = attempts;
      return result;
    }
  }

  SDFCreationResult result;
  result.report = evaluateSDFCreationConstraints(
      constraints, last_measurements, attempts);
  attachFeatureEvidence(&result.report, features, advice);
  if (last_measurements.sdf_file_bytes == 0 &&
      last_measurements.max_decoded_block_bytes == 0 &&
      last_measurements.max_zero_surface_abs_error == 0.0) {
    result.report.status = SDFCreationStatus::BuildFailed;
    result.report.message = "all constrained SDF build attempts failed before validation";
  } else {
    result.report.status = SDFCreationStatus::Infeasible;
    result.report.message =
        "no attempted backend parameters satisfied all hard constraints";
  }
  return result;
}

}  // namespace

SDFCreationResult ConstrainedSDFBuilder::fromMesh(
    const TriangleMesh& mesh,
    const std::filesystem::path& output_path,
    const SDFCreationConstraints& constraints) {
  std::string error;
  if (!validateSDFCreationConstraints(constraints, &error)) {
    return invalidResult(
        constraints, SDFCreationStatus::InvalidInput, error);
  }
  if (output_path.empty()) {
    return invalidResult(
        constraints,
        SDFCreationStatus::InvalidInput,
        "output path must not be empty");
  }
  if (constraints.max_decoded_block_bytes < 128) {
    SDFCreationResult result;
    result.report = minimumMemoryInfeasible(constraints);
    return result;
  }
  const Clock::time_point feature_begin = Clock::now();
  const MeshDiagnosticsReport diagnostics = MeshDiagnostics::analyze(mesh);
  const GeometryFeatures features =
      GeometryFeatureExtractor::fromMesh(mesh, diagnostics);
  const double feature_time_ms = elapsedMs(feature_begin);
  if (!features.valid) {
    return invalidResult(
        constraints,
        SDFCreationStatus::InvalidInput,
        "signed SDF creation requires a valid watertight mesh");
  }
  const Clock::time_point advisor_begin = Clock::now();
  const BackendParameterAdvice advice =
      selectBackendAdvice(features, constraints);
  const double advisor_time_ms = elapsedMs(advisor_begin);
  if (!advice.success) {
    return invalidResult(
        constraints,
        SDFCreationStatus::BuildFailed,
        "backend parameter advisor did not produce valid parameters");
  }
  if (!validateBackendBuildParameters(advice.parameters, &error)) {
    return invalidResult(
        constraints,
        SDFCreationStatus::BuildFailed,
        "backend parameter advisor produced invalid parameters: " + error);
  }
  const Clock::time_point search_begin = Clock::now();
  SDFCreationResult result = buildConstrainedSDF(
      mesh, output_path, constraints, features, advice, false);
  result.report.mesh_feature_time_ms = feature_time_ms;
  result.report.advisor_time_ms = advisor_time_ms;
  result.report.search_time_ms = elapsedMs(search_begin);
  return result;
}

SDFCreationResult ConstrainedSDFBuilder::fromSTLWithParameters(
    const std::filesystem::path& stl_path,
    const std::filesystem::path& output_path,
    const SDFCreationConstraints& constraints,
    const BackendBuildParameters& parameters) {
  std::string error;
  if (!validateSDFCreationConstraints(constraints, &error)) {
    return invalidResult(
        constraints, SDFCreationStatus::InvalidInput, error);
  }
  if (!validateBackendBuildParameters(parameters, &error)) {
    return invalidResult(
        constraints, SDFCreationStatus::InvalidInput, error);
  }
  const STLReadResult read = STLReader::read(stl_path.string());
  if (!read.success) {
    return invalidResult(
        constraints,
        SDFCreationStatus::InvalidInput,
        "failed to read STL: " + read.error_message);
  }
  return fromMeshWithParameters(
      read.mesh, output_path, constraints, parameters);
}

SDFCreationResult ConstrainedSDFBuilder::fromMeshWithParameters(
    const TriangleMesh& mesh,
    const std::filesystem::path& output_path,
    const SDFCreationConstraints& constraints,
    const BackendBuildParameters& parameters) {
  std::string error;
  if (!validateSDFCreationConstraints(constraints, &error)) {
    return invalidResult(
        constraints, SDFCreationStatus::InvalidInput, error);
  }
  if (!validateBackendBuildParameters(parameters, &error)) {
    return invalidResult(
        constraints, SDFCreationStatus::InvalidInput, error);
  }
  if (output_path.empty()) {
    return invalidResult(
        constraints,
        SDFCreationStatus::InvalidInput,
        "output path must not be empty");
  }
  if (constraints.max_decoded_block_bytes < 128) {
    SDFCreationResult result;
    result.report = minimumMemoryInfeasible(constraints);
    return result;
  }
  const Clock::time_point feature_begin = Clock::now();
  const MeshDiagnosticsReport diagnostics = MeshDiagnostics::analyze(mesh);
  const GeometryFeatures features =
      GeometryFeatureExtractor::fromMesh(mesh, diagnostics);
  const double feature_time_ms = elapsedMs(feature_begin);
  if (!features.valid) {
    return invalidResult(
        constraints,
        SDFCreationStatus::InvalidInput,
        "signed SDF creation requires a valid watertight mesh");
  }
  BackendParameterAdvice advice;
  advice.success = true;
  advice.source = "explicit-v1";
  advice.parameters = parameters;
  const Clock::time_point search_begin = Clock::now();
  SDFCreationResult result = buildConstrainedSDF(
      mesh, output_path, constraints, features, advice, true);
  result.report.mesh_feature_time_ms = feature_time_ms;
  result.report.advisor_time_ms = 0.0;
  result.report.search_time_ms = elapsedMs(search_begin);
  return result;
}

}  // namespace adasdf
