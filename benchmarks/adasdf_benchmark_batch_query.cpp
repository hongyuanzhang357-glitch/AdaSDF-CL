#include <adasdf/adasdf.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct SeriesStats {
  double min = 0.0;
  double mean = 0.0;
  double max = 0.0;
  double stddev = 0.0;
};

struct BenchmarkRow {
  std::string query_backend;
  std::string expansion_mode;
  std::string selected_blocks;
  std::size_t num_points = 0;
  double expanded_memory_mb = 0.0;
  double gpu_resident_memory_mb = 0.0;

  adasdf::BatchQueryTiming timing;
  double query_kernel_ms = 0.0;
  double query_total_ms = 0.0;
  double ns_per_query = 0.0;
  double queries_per_second = 0.0;
  std::size_t fallback_count = 0;

  double max_abs_phi_error = 0.0;
  double max_normal_error = 0.0;
  bool max_normal_error_na = false;
  double max_abs_error = 0.0;
  double mean_abs_error = 0.0;
  double rms_error = 0.0;
  double p95_abs_error = 0.0;
  int sign_mismatch_count = 0;
  double sign_mismatch_rate = 0.0;
  int ambiguous_sign_count = 0;
  double ambiguous_sign_rate = 0.0;
  int near_surface_sign_mismatch_count = 0;
  double near_surface_sign_mismatch_rate = 0.0;
  double fallback_rate = 0.0;

  int warmup = 0;
  int repeat = 1;
  SeriesStats kernel_stats;
  SeriesStats total_stats;
  bool kernel_only = false;
  bool phi_only = false;
  std::string output_mode = "phi,normal";
  bool reuse_resident = false;
  bool download_results = true;
  bool correctness_checked = true;
  std::string host_memory = "paged";
  std::string layout = "aos";
  bool workspace_reused = false;
  std::size_t allocation_count = 0;
  std::size_t workspace_capacity = 0;
  double workspace_device_memory_mb = 0.0;
  std::size_t block_lookup_count = 0;
  std::size_t block_scan_count = 0;
  double center_block_hit_rate = 0.0;
  double neighbor_same_block_rate = 0.0;

  bool cuda_available = false;
  std::string status = "ok";
  std::string error_message;
  bool skipped = false;
};

std::vector<std::string> splitList(const std::string& text) {
  std::vector<std::string> values;
  std::stringstream stream(text);
  std::string item;
  while (std::getline(stream, item, ',')) {
    if (!item.empty()) {
      values.push_back(item);
    }
  }
  return values;
}

std::vector<std::size_t> parsePointCounts(const std::string& text) {
  std::vector<std::size_t> values;
  for (const std::string& item : splitList(text)) {
    const auto count = static_cast<std::size_t>(std::stoull(item));
    if (count == 0) {
      throw std::runtime_error("point counts must be positive");
    }
    values.push_back(count);
  }
  if (values.empty()) {
    throw std::runtime_error("at least one point count is required");
  }
  return values;
}

adasdf::QueryBackend parseBackend(const std::string& text) {
  if (text == "cpu") {
    return adasdf::QueryBackend::CPU;
  }
  if (text == "cuda") {
    return adasdf::QueryBackend::CUDA;
  }
  if (text == "auto") {
    return adasdf::QueryBackend::Auto;
  }
  throw std::runtime_error("query backend must be cpu, cuda, or auto");
}

adasdf::QueryExpansionMode parseExpansion(const std::string& text) {
  if (text == "none") {
    return adasdf::QueryExpansionMode::None;
  }
  if (text == "global") {
    return adasdf::QueryExpansionMode::Global;
  }
  if (text == "block") {
    return adasdf::QueryExpansionMode::Block;
  }
  if (text == "auto") {
    return adasdf::QueryExpansionMode::Auto;
  }
  throw std::runtime_error("expansion must be none, global, block, or auto");
}

adasdf::BlockSelection parseBlocks(const std::string& text) {
  if (text.empty() || text == "all") {
    return adasdf::BlockSelection::all();
  }
  std::vector<int> ids;
  for (const std::string& item : splitList(text)) {
    ids.push_back(std::stoi(item));
  }
  return adasdf::BlockSelection::selected(std::move(ids));
}

adasdf::QueryOutputMode parseOutputMode(const std::string& text) {
  if (text == "phi") {
    return adasdf::QueryOutputMode::PhiOnly;
  }
  if (text == "phi,normal" || text == "phi+normal" || text == "full") {
    return adasdf::QueryOutputMode::PhiAndNormal;
  }
  throw std::runtime_error("output must be phi or phi,normal");
}

double vectorError(const adasdf::Vector3& a, const adasdf::Vector3& b) {
  const double dx = a.x - b.x;
  const double dy = a.y - b.y;
  const double dz = a.z - b.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

void computeErrors(
    const adasdf::BatchQueryOutput& reference,
    const adasdf::BatchQueryOutput& candidate,
    bool compare_normals,
    double& max_phi_error,
    double& max_normal_error) {
  max_phi_error = 0.0;
  max_normal_error = 0.0;
  const std::size_t count = std::min(
      reference.signed_distances.size(),
      candidate.signed_distances.size());
  for (std::size_t i = 0; i < count; ++i) {
    if (!std::isfinite(candidate.signed_distances[i])) {
      max_phi_error = std::numeric_limits<double>::infinity();
      max_normal_error = std::numeric_limits<double>::infinity();
      return;
    }
    max_phi_error = std::max(
        max_phi_error,
        std::abs(reference.signed_distances[i] - candidate.signed_distances[i]));
    if (compare_normals) {
      max_normal_error = std::max(
          max_normal_error,
          vectorError(reference.normals[i], candidate.normals[i]));
    }
  }
}

double percentile95(std::vector<double> values) {
  if (values.empty()) {
    return 0.0;
  }
  std::sort(values.begin(), values.end());
  const double rank = 0.95 * static_cast<double>(values.size() - 1);
  const auto index = static_cast<std::size_t>(std::ceil(rank));
  return values[std::min(index, values.size() - 1)];
}

adasdf::ExpansionQualityReport computeQualityFromOutputs(
    const std::vector<adasdf::Vector3>& points,
    const adasdf::BatchQueryOutput& reference,
    const adasdf::BatchQueryOutput& candidate,
    double near_surface_band,
    double sign_epsilon,
    std::size_t fallback_count) {
  adasdf::ExpansionQualityReport report;
  report.num_samples = static_cast<int>(std::min(
      reference.signed_distances.size(),
      candidate.signed_distances.size()));
  report.fallback_count = static_cast<int>(fallback_count);

  std::vector<double> abs_errors;
  abs_errors.reserve(static_cast<std::size_t>(report.num_samples));
  double sum_abs = 0.0;
  double sum_sq = 0.0;

  for (int i = 0; i < report.num_samples; ++i) {
    const double direct_phi = reference.signed_distances[static_cast<std::size_t>(i)];
    const double expanded_phi = candidate.signed_distances[static_cast<std::size_t>(i)];
    if (!std::isfinite(direct_phi) || !std::isfinite(expanded_phi)) {
      continue;
    }
    const double abs_error = std::abs(direct_phi - expanded_phi);
    abs_errors.push_back(abs_error);
    ++report.num_finite_samples;
    sum_abs += abs_error;
    sum_sq += abs_error * abs_error;
    if (abs_error > report.max_abs_error) {
      report.max_abs_error = abs_error;
      report.worst_point_id = i;
      if (static_cast<std::size_t>(i) < points.size()) {
        report.worst_point = points[static_cast<std::size_t>(i)];
      }
      report.worst_direct_phi = direct_phi;
      report.worst_expanded_phi = expanded_phi;
    }

    const adasdf::SDFSignClass direct_sign =
        adasdf::classifySDFSign(direct_phi, sign_epsilon);
    const adasdf::SDFSignClass expanded_sign =
        adasdf::classifySDFSign(expanded_phi, sign_epsilon);
    if (direct_sign == adasdf::SDFSignClass::Ambiguous ||
        expanded_sign == adasdf::SDFSignClass::Ambiguous) {
      ++report.ambiguous_sign_count;
    } else if (adasdf::isStrictSignMismatch(
                   direct_phi,
                   expanded_phi,
                   sign_epsilon)) {
      ++report.sign_mismatch_count;
    }
    if (std::abs(direct_phi) <= near_surface_band) {
      ++report.near_surface_sample_count;
      if (adasdf::isStrictSignMismatch(
              direct_phi,
              expanded_phi,
              sign_epsilon)) {
        ++report.near_surface_sign_mismatch_count;
      }
    }
  }

  if (report.num_finite_samples > 0) {
    const double finite = static_cast<double>(report.num_finite_samples);
    report.mean_abs_error = sum_abs / finite;
    report.rms_error = std::sqrt(sum_sq / finite);
    report.p95_abs_error = percentile95(abs_errors);
    report.sign_mismatch_rate =
        static_cast<double>(report.sign_mismatch_count) / finite;
    report.ambiguous_sign_rate =
        static_cast<double>(report.ambiguous_sign_count) / finite;
  }
  if (report.near_surface_sample_count > 0) {
    report.near_surface_sign_mismatch_rate =
        static_cast<double>(report.near_surface_sign_mismatch_count) /
        static_cast<double>(report.near_surface_sample_count);
  }
  if (report.num_samples > 0) {
    report.fallback_rate =
        static_cast<double>(report.fallback_count) /
        static_cast<double>(report.num_samples);
  }
  return report;
}

SeriesStats summarize(const std::vector<double>& values) {
  SeriesStats stats;
  if (values.empty()) {
    return stats;
  }
  stats.min = *std::min_element(values.begin(), values.end());
  stats.max = *std::max_element(values.begin(), values.end());
  stats.mean = std::accumulate(values.begin(), values.end(), 0.0) /
               static_cast<double>(values.size());
  double sum_sq = 0.0;
  for (const double value : values) {
    const double delta = value - stats.mean;
    sum_sq += delta * delta;
  }
  stats.stddev = std::sqrt(sum_sq / static_cast<double>(values.size()));
  return stats;
}

adasdf::BatchQueryTiming meanTiming(
    const std::vector<adasdf::BatchQueryTiming>& timings) {
  adasdf::BatchQueryTiming out;
  if (timings.empty()) {
    return out;
  }
  for (const adasdf::BatchQueryTiming& timing : timings) {
    out.setup_ms += timing.setup_ms;
    out.expand_ms += timing.expand_ms;
    out.upload_sdf_ms += timing.upload_sdf_ms;
    out.h2d_points_ms += timing.h2d_points_ms;
    out.kernel_ms += timing.kernel_ms;
    out.d2h_results_ms += timing.d2h_results_ms;
    out.sync_ms += timing.sync_ms;
    out.postprocess_ms += timing.postprocess_ms;
    out.allocation_ms += timing.allocation_ms;
    out.free_ms += timing.free_ms;
    out.total_ms += timing.total_ms;
  }
  const double n = static_cast<double>(timings.size());
  out.setup_ms /= n;
  out.expand_ms /= n;
  out.upload_sdf_ms /= n;
  out.h2d_points_ms /= n;
  out.kernel_ms /= n;
  out.d2h_results_ms /= n;
  out.sync_ms /= n;
  out.postprocess_ms /= n;
  out.allocation_ms /= n;
  out.free_ms /= n;
  out.total_ms /= n;
  const adasdf::BatchQueryTiming& last = timings.back();
  out.workspace_reused = last.workspace_reused;
  out.allocation_count = last.allocation_count;
  out.workspace_capacity = last.workspace_capacity;
  out.workspace_device_memory_mb = last.workspace_device_memory_mb;
  out.block_lookup_count = last.block_lookup_count;
  out.block_scan_count = last.block_scan_count;
  out.center_block_hit_rate = last.center_block_hit_rate;
  out.neighbor_same_block_rate = last.neighbor_same_block_rate;
  out.download_results = last.download_results;
  out.correctness_checked = last.correctness_checked;
  return out;
}

std::string numberOrBlank(double value, bool blank) {
  if (blank) {
    return "";
  }
  if (!std::isfinite(value)) {
    return "inf";
  }
  std::ostringstream stream;
  stream << std::setprecision(10) << value;
  return stream.str();
}

std::string numberOrNA(double value, bool na) {
  if (na) {
    return "NA";
  }
  return numberOrBlank(value, false);
}

std::string csvField(const std::string& value) {
  if (value.find_first_of(",\"\n\r") == std::string::npos) {
    return value;
  }
  std::string escaped = "\"";
  for (const char ch : value) {
    if (ch == '"') {
      escaped += "\"\"";
    } else {
      escaped += ch;
    }
  }
  escaped += "\"";
  return escaped;
}

void writeCsv(std::ostream& out, const std::vector<BenchmarkRow>& rows) {
  out << "query_backend,expansion_mode,selected_blocks,num_points,"
         "expanded_memory_mb,gpu_resident_memory_mb,"
         "setup_ms,expand_ms,upload_sdf_ms,allocation_ms,h2d_points_ms,"
         "kernel_ms,sync_ms,d2h_results_ms,postprocess_ms,free_ms,total_ms,"
         "query_kernel_ms,query_total_ms,ns_per_query,queries_per_second,"
         "fallback_count,max_abs_phi_error,max_normal_error,cuda_available,"
         "max_abs_error,mean_abs_error,rms_error,p95_abs_error,"
         "sign_mismatch_count,sign_mismatch_rate,ambiguous_sign_count,"
         "ambiguous_sign_rate,near_surface_sign_mismatch_count,"
         "near_surface_sign_mismatch_rate,fallback_rate,"
         "warmup,repeat,kernel_min_ms,kernel_mean_ms,kernel_max_ms,"
         "kernel_std_ms,total_min_ms,total_mean_ms,total_max_ms,total_std_ms,"
         "output_mode,phi_only,reuse_resident,kernel_only,"
         "workspace_reused,allocation_count,workspace_capacity,"
         "workspace_device_memory_mb,block_lookup_count,block_scan_count,"
         "center_block_hit_rate,neighbor_same_block_rate,"
         "download_results,correctness_checked,host_memory,layout,"
         "status,error_message\n";
  for (const BenchmarkRow& row : rows) {
    const bool cuda_kernel_na = row.query_backend != "cuda" || row.skipped;
    out << csvField(row.query_backend) << "," << csvField(row.expansion_mode) << ","
        << csvField(row.selected_blocks) << "," << row.num_points << ","
        << numberOrBlank(row.expanded_memory_mb, row.skipped) << ","
        << numberOrBlank(row.gpu_resident_memory_mb, row.skipped) << ","
        << numberOrBlank(row.timing.setup_ms, row.skipped) << ","
        << numberOrBlank(row.timing.expand_ms, row.skipped) << ","
        << numberOrBlank(row.timing.upload_sdf_ms, row.skipped) << ","
        << numberOrBlank(row.timing.allocation_ms, row.skipped) << ","
        << numberOrBlank(row.timing.h2d_points_ms, row.skipped) << ","
        << numberOrNA(row.timing.kernel_ms, cuda_kernel_na) << ","
        << numberOrNA(row.timing.sync_ms, cuda_kernel_na) << ","
        << numberOrBlank(row.timing.d2h_results_ms, row.skipped) << ","
        << numberOrBlank(row.timing.postprocess_ms, row.skipped) << ","
        << numberOrBlank(row.timing.free_ms, row.skipped) << ","
        << numberOrBlank(row.timing.total_ms, row.skipped) << ","
        << numberOrNA(row.query_kernel_ms, cuda_kernel_na) << ","
        << numberOrBlank(row.query_total_ms, row.skipped) << ","
        << numberOrBlank(row.ns_per_query, row.skipped) << ","
        << numberOrBlank(row.queries_per_second, row.skipped) << ","
        << row.fallback_count << ","
        << numberOrBlank(row.max_abs_phi_error, row.skipped) << ","
        << numberOrNA(row.max_normal_error, row.max_normal_error_na || row.skipped) << ","
        << (row.cuda_available ? "true" : "false") << ","
        << numberOrBlank(row.max_abs_error, row.skipped) << ","
        << numberOrBlank(row.mean_abs_error, row.skipped) << ","
        << numberOrBlank(row.rms_error, row.skipped) << ","
        << numberOrBlank(row.p95_abs_error, row.skipped) << ","
        << row.sign_mismatch_count << ","
        << numberOrBlank(row.sign_mismatch_rate, row.skipped) << ","
        << row.ambiguous_sign_count << ","
        << numberOrBlank(row.ambiguous_sign_rate, row.skipped) << ","
        << row.near_surface_sign_mismatch_count << ","
        << numberOrBlank(row.near_surface_sign_mismatch_rate, row.skipped) << ","
        << numberOrBlank(row.fallback_rate, row.skipped) << ","
        << row.warmup << "," << row.repeat << ","
        << numberOrNA(row.kernel_stats.min, cuda_kernel_na) << ","
        << numberOrNA(row.kernel_stats.mean, cuda_kernel_na) << ","
        << numberOrNA(row.kernel_stats.max, cuda_kernel_na) << ","
        << numberOrNA(row.kernel_stats.stddev, cuda_kernel_na) << ","
        << numberOrBlank(row.total_stats.min, row.skipped) << ","
        << numberOrBlank(row.total_stats.mean, row.skipped) << ","
        << numberOrBlank(row.total_stats.max, row.skipped) << ","
        << numberOrBlank(row.total_stats.stddev, row.skipped) << ","
        << csvField(row.output_mode) << ","
        << (row.phi_only ? "true" : "false") << ","
        << (row.reuse_resident ? "true" : "false") << ","
        << (row.kernel_only ? "true" : "false") << ","
        << (row.workspace_reused ? "true" : "false") << ","
        << row.allocation_count << ","
        << row.workspace_capacity << ","
        << numberOrBlank(row.workspace_device_memory_mb, row.skipped) << ","
        << row.block_lookup_count << ","
        << row.block_scan_count << ","
        << numberOrBlank(row.center_block_hit_rate, row.skipped) << ","
        << numberOrBlank(row.neighbor_same_block_rate, row.skipped) << ","
        << (row.download_results ? "true" : "false") << ","
        << (row.correctness_checked ? "true" : "false") << ","
        << csvField(row.host_memory) << ","
        << csvField(row.layout) << ","
        << csvField(row.status) << "," << csvField(row.error_message) << "\n";
  }
}

adasdf::AABB blockDomain(
    const adasdf::SDFModel& model,
    const adasdf::BlockSelection& selection) {
  if (selection.use_all_blocks || model.blockMetadata().empty()) {
    return model.boundingBox();
  }

  adasdf::AABB domain;
  domain.valid = false;
  for (const adasdf::SDFBlockMetadata& block : model.blockMetadata()) {
    if (std::find(
            selection.block_ids.begin(),
            selection.block_ids.end(),
            static_cast<int>(block.block_id)) == selection.block_ids.end()) {
      continue;
    }
    if (!domain.valid) {
      domain.min = block.local_min;
      domain.max = block.local_max;
      domain.valid = true;
    } else {
      domain.min.x = std::min(domain.min.x, block.local_min.x);
      domain.min.y = std::min(domain.min.y, block.local_min.y);
      domain.min.z = std::min(domain.min.z, block.local_min.z);
      domain.max.x = std::max(domain.max.x, block.local_max.x);
      domain.max.y = std::max(domain.max.y, block.local_max.y);
      domain.max.z = std::max(domain.max.z, block.local_max.z);
    }
  }
  if (!domain.valid) {
    throw std::runtime_error("selected benchmark blocks do not exist");
  }
  return domain;
}

std::vector<adasdf::Vector3> makePoints(
    const adasdf::SDFModel& model,
    const adasdf::BlockSelection& selection,
    std::size_t count) {
  const adasdf::AABB domain = blockDomain(model, selection);
  adasdf::PointCloudGeneratorOptions options;
  options.num_points = count;
  options.distribution = adasdf::BenchmarkPointDistribution::UniformBoxVolume;
  options.seed = 1337;
  options.center = 0.5 * (domain.min + domain.max);
  options.half_extent = 0.5 * (domain.max - domain.min);
  options.volume_scale = 0.9;
  return adasdf::generateBenchmarkPoints(options);
}

void printSummary(const std::vector<BenchmarkRow>& rows) {
  std::cout
      << "backend | expansion | output | blocks | points | setup ms | total mean ms | "
         "kernel mean ms | ns/query | max phi error | max normal error | status\n";
  for (const BenchmarkRow& row : rows) {
    std::cout << row.query_backend << " | " << row.expansion_mode << " | "
              << row.output_mode << " | " << row.selected_blocks << " | "
              << row.num_points << " | ";
    if (row.skipped) {
      std::cout << "SKIPPED | SKIPPED | SKIPPED | SKIPPED | SKIPPED | "
                << "SKIPPED | " << row.status << "\n";
      continue;
    }
    std::cout << row.timing.setup_ms << " | " << row.total_stats.mean << " | ";
    if (row.query_backend == "cuda") {
      std::cout << row.kernel_stats.mean;
    } else {
      std::cout << "NA";
    }
    std::cout << " | " << row.ns_per_query << " | "
              << row.max_abs_phi_error << " | ";
    if (row.max_normal_error_na) {
      std::cout << "NA";
    } else {
      std::cout << row.max_normal_error;
    }
    std::cout << " | " << row.status << "\n";
  }
}

void applySetupTiming(BenchmarkRow& row, const adasdf::BatchQueryTiming& setup) {
  row.timing.setup_ms = setup.setup_ms;
  row.timing.expand_ms = setup.expand_ms;
  row.timing.upload_sdf_ms = setup.upload_sdf_ms;
}

void finishRowFromRuns(
    BenchmarkRow& row,
    const std::vector<adasdf::BatchQueryTiming>& timings) {
  row.timing.allocation_ms = 0.0;
  row.timing.h2d_points_ms = 0.0;
  row.timing.kernel_ms = 0.0;
  row.timing.sync_ms = 0.0;
  row.timing.d2h_results_ms = 0.0;
  row.timing.postprocess_ms = 0.0;
  row.timing.free_ms = 0.0;
  row.timing.total_ms = 0.0;

  const adasdf::BatchQueryTiming mean = meanTiming(timings);
  row.timing.allocation_ms = mean.allocation_ms;
  row.timing.h2d_points_ms = mean.h2d_points_ms;
  row.timing.kernel_ms = mean.kernel_ms;
  row.timing.sync_ms = mean.sync_ms;
  row.timing.d2h_results_ms = mean.d2h_results_ms;
  row.timing.postprocess_ms = mean.postprocess_ms;
  row.timing.free_ms = mean.free_ms;
  row.timing.total_ms = mean.total_ms;
  row.timing.workspace_reused = mean.workspace_reused;
  row.timing.allocation_count = mean.allocation_count;
  row.timing.workspace_capacity = mean.workspace_capacity;
  row.timing.workspace_device_memory_mb = mean.workspace_device_memory_mb;
  row.timing.block_lookup_count = mean.block_lookup_count;
  row.timing.block_scan_count = mean.block_scan_count;
  row.timing.center_block_hit_rate = mean.center_block_hit_rate;
  row.timing.neighbor_same_block_rate = mean.neighbor_same_block_rate;
  row.timing.download_results = mean.download_results;
  row.timing.correctness_checked = mean.correctness_checked;
  row.workspace_reused = mean.workspace_reused;
  row.allocation_count = mean.allocation_count;
  row.workspace_capacity = mean.workspace_capacity;
  row.workspace_device_memory_mb = mean.workspace_device_memory_mb;
  row.block_lookup_count = mean.block_lookup_count;
  row.block_scan_count = mean.block_scan_count;
  row.center_block_hit_rate = mean.center_block_hit_rate;
  row.neighbor_same_block_rate = mean.neighbor_same_block_rate;
  row.download_results = mean.download_results;
  row.correctness_checked = mean.correctness_checked;
  row.query_kernel_ms = mean.kernel_ms;
  row.query_total_ms = mean.total_ms;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    std::string points_arg = "10000";
    std::string backend_arg = "cpu";
    std::string expansion_arg = "none";
    std::string blocks_arg = "all";
    bool expansion_was_set = false;
    int global_resolution = 64;
    int block_resolution = 32;
    double near_surface_band = 1e-3;
    double sign_epsilon = 1e-9;
    bool keep_resident = true;
    bool reuse_resident = false;
    bool kernel_only = false;
    std::string output_arg = "phi,normal";
    bool device_only = false;
    int warmup = 0;
    int repeat = 1;
    std::filesystem::path model_path;
    std::filesystem::path output_path;

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--points" && i + 1 < argc) {
        points_arg = argv[++i];
      } else if ((arg == "--query-backend" || arg == "--backend") &&
                 i + 1 < argc) {
        backend_arg = argv[++i];
      } else if (arg == "--expansion" && i + 1 < argc) {
        expansion_arg = argv[++i];
        expansion_was_set = true;
      } else if (arg == "--blocks" && i + 1 < argc) {
        blocks_arg = argv[++i];
      } else if (arg == "--global-resolution" && i + 1 < argc) {
        global_resolution = std::stoi(argv[++i]);
      } else if (arg == "--block-resolution" && i + 1 < argc) {
        block_resolution = std::stoi(argv[++i]);
      } else if (arg == "--near-surface-band" && i + 1 < argc) {
        near_surface_band = std::stod(argv[++i]);
      } else if (arg == "--sign-epsilon" && i + 1 < argc) {
        sign_epsilon = std::stod(argv[++i]);
      } else if (arg == "--warmup" && i + 1 < argc) {
        warmup = std::stoi(argv[++i]);
      } else if (arg == "--repeat" && i + 1 < argc) {
        repeat = std::stoi(argv[++i]);
      } else if (arg == "--kernel-only") {
        kernel_only = true;
      } else if (arg == "--reuse-resident") {
        reuse_resident = true;
      } else if (arg == "--output" && i + 1 < argc) {
        output_arg = argv[++i];
      } else if (arg == "--phi-only") {
        output_arg = "phi";
      } else if (arg == "--device-only" || arg == "--no-download") {
        device_only = true;
      } else if (arg == "--keep-resident") {
        keep_resident = true;
      } else if (arg == "--model" && i + 1 < argc) {
        model_path = argv[++i];
      } else if (arg == "--out" && i + 1 < argc) {
        output_path = argv[++i];
      } else if (arg == "--help" || arg == "-h") {
        std::cout
            << "Usage: adasdf_benchmark_batch_query --points 10000,1000000 "
               "--query-backend cpu|cuda --expansion none|global|block "
               "[--blocks all|0,1,2] [--global-resolution 64] "
               "[--block-resolution 32] [--warmup N] [--repeat N] "
               "[--kernel-only] [--reuse-resident] "
               "[--output phi|phi,normal] [--phi-only] [--device-only] "
               "[--model model.sdfbin] "
               "[--near-surface-band 1e-3] [--sign-epsilon 1e-9] "
               "[--keep-resident] [--out benchmark.csv]\n";
        return 0;
      } else {
        throw std::runtime_error("unknown or incomplete argument: " + arg);
      }
    }

    if (warmup < 0) {
      throw std::runtime_error("--warmup must be non-negative");
    }
    if (repeat < 1) {
      throw std::runtime_error("--repeat must be at least 1");
    }

    const std::vector<std::size_t> point_counts = parsePointCounts(points_arg);
    const std::vector<std::string> backend_names = splitList(backend_arg);
    const adasdf::BlockSelection block_selection = parseBlocks(blocks_arg);
    const adasdf::QueryOutputMode output_mode = parseOutputMode(output_arg);
    const bool phi_only = output_mode == adasdf::QueryOutputMode::PhiOnly;
    const bool download_results = !device_only;
    const bool cuda_available = adasdf::CudaQueryBackend::isAvailable();

    std::shared_ptr<adasdf::SDFModel> model;
    if (!model_path.empty()) {
      if (!std::filesystem::exists(model_path)) {
        throw std::runtime_error("benchmark --model path does not exist: " +
                                 model_path.string());
      }
      model = adasdf::SDFBinReader::read(model_path);
      if (!model || !model->isValid() || !model->queryBackendAvailable()) {
        throw std::runtime_error(
            "benchmark --model must load a valid queryable SDFModel");
      }
    } else {
      adasdf::DemoAdaptiveBuildRequest build_request;
      build_request.use_surrogate = false;
      const auto build = adasdf::DemoAdaptiveSDFBuilder::build(build_request);
      if (!build.model) {
        throw std::runtime_error("failed to create demo adaptive benchmark model");
      }
      model = build.model;
    }

    std::vector<BenchmarkRow> rows;
    for (const std::string& backend_name : backend_names) {
      const adasdf::QueryBackend backend = parseBackend(backend_name);
      std::string effective_expansion_arg = expansion_arg;
      if (!expansion_was_set) {
        effective_expansion_arg =
            backend == adasdf::QueryBackend::CUDA ? "global" : "none";
      }
      const adasdf::QueryExpansionMode expansion =
          parseExpansion(effective_expansion_arg);

      for (const std::size_t count : point_counts) {
        BenchmarkRow row;
        row.query_backend = adasdf::toString(backend);
        row.expansion_mode = adasdf::toString(expansion);
        row.selected_blocks = adasdf::blockSelectionString(block_selection);
        row.num_points = count;
        row.cuda_available = cuda_available;
        row.warmup = warmup;
        row.repeat = repeat;
        row.kernel_only = kernel_only;
        row.phi_only = phi_only;
        row.output_mode = adasdf::toString(output_mode);
        row.reuse_resident = reuse_resident;
        row.download_results = download_results;
        row.correctness_checked = download_results;
        row.max_normal_error_na = phi_only || !download_results;

        if (kernel_only && backend != adasdf::QueryBackend::CUDA) {
          row.status = "skipped";
          row.error_message = "kernel-only timing is only available for CUDA";
          row.skipped = true;
          rows.push_back(row);
          continue;
        }

        if (device_only && backend != adasdf::QueryBackend::CUDA) {
          row.status = "skipped";
          row.error_message = "device-only benchmark mode is only available for CUDA";
          row.skipped = true;
          rows.push_back(row);
          continue;
        }

        if (backend == adasdf::QueryBackend::CUDA && !cuda_available) {
          row.status = "skipped";
          row.error_message = "CUDA backend unavailable";
          row.skipped = true;
          rows.push_back(row);
          continue;
        }

        try {
          adasdf::ExpansionOptions expansion_options;
          expansion_options.expansion = expansion;
          expansion_options.block_selection = block_selection;
          expansion_options.global_resolution = global_resolution;
          expansion_options.block_resolution = block_resolution;
          expansion_options.padding = 0.0;
          expansion_options.near_surface_band = near_surface_band;
          expansion_options.sign_epsilon = sign_epsilon;

          const std::vector<adasdf::Vector3> points =
              makePoints(*model, block_selection, count);
          adasdf::BatchQueryOutput reference;
          if (download_results) {
            reference = adasdf::queryBatchCPU(
                *model,
                points,
                adasdf::QueryOutputMode::PhiAndNormal);
          }

          std::vector<adasdf::BatchQueryTiming> timings;
          std::vector<double> kernel_values;
          std::vector<double> total_values;
          adasdf::BatchQueryOutput output;

          if (backend == adasdf::QueryBackend::CUDA) {
            if (expansion == adasdf::QueryExpansionMode::None) {
              throw std::runtime_error("CUDA benchmark requires global or block expansion");
            }
            adasdf::BatchQueryTiming setup_timing;
            const auto setup0 = std::chrono::steady_clock::now();
            const auto expand0 = std::chrono::steady_clock::now();
            adasdf::ExpandedSDF expanded =
                adasdf::SDFExpander::expand(*model, expansion_options);
            const auto expand1 = std::chrono::steady_clock::now();
            setup_timing.expand_ms =
                std::chrono::duration<double, std::milli>(expand1 - expand0).count();
            row.expanded_memory_mb =
                static_cast<double>(expanded.memoryFootprintBytes()) /
                (1024.0 * 1024.0);

            adasdf::CudaResidentExpandedSDF resident;
            const auto upload0 = std::chrono::steady_clock::now();
            if (!resident.upload(expanded)) {
              throw std::runtime_error("CUDA resident expanded SDF upload failed");
            }
            const auto upload1 = std::chrono::steady_clock::now();
            setup_timing.upload_sdf_ms =
                std::chrono::duration<double, std::milli>(upload1 - upload0).count();
            setup_timing.setup_ms =
                std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now() - setup0)
                    .count();
            applySetupTiming(row, setup_timing);

            adasdf::CudaQueryWorkspace workspace;
            adasdf::CudaQueryWorkspace* workspace_ptr = nullptr;
            if (reuse_resident) {
              if (!workspace.ensureCapacity(count, adasdf::includesNormals(output_mode))) {
                throw std::runtime_error("CUDA query workspace allocation failed");
              }
              workspace_ptr = &workspace;
            }
            row.gpu_resident_memory_mb =
                static_cast<double>(
                    resident.deviceMemoryBytes() +
                    (workspace_ptr != nullptr ? workspace.deviceMemoryBytes() : 0)) /
                (1024.0 * 1024.0);

            for (int i = 0; i < warmup; ++i) {
              adasdf::BatchQueryTiming ignored;
              resident.queryBatchInto(
                  points,
                  output_mode,
                  workspace_ptr,
                  download_results ? &output : nullptr,
                  &ignored,
                  download_results);
            }

            for (int i = 0; i < repeat; ++i) {
              adasdf::BatchQueryTiming timing;
              resident.queryBatchInto(
                  points,
                  output_mode,
                  workspace_ptr,
                  download_results ? &output : nullptr,
                  &timing,
                  download_results);
              timings.push_back(timing);
              kernel_values.push_back(timing.kernel_ms);
              total_values.push_back(timing.total_ms);
            }
          } else {
            adasdf::QueryModeConfig config;
            config.backend = backend;
            config.expansion = expansion;
            config.block_selection = block_selection;
            config.keep_expanded_data_resident = keep_resident;
            config.allow_fallback_to_cpu = true;

            if (expansion == adasdf::QueryExpansionMode::None) {
              for (int i = 0; i < warmup; ++i) {
                output = adasdf::queryBatchCPU(*model, points, output_mode);
              }
              for (int i = 0; i < repeat; ++i) {
                adasdf::BatchQueryTiming timing;
                output = adasdf::queryBatchCPU(
                    *model,
                    points,
                    output_mode,
                    nullptr,
                    &timing);
                timings.push_back(timing);
                total_values.push_back(timing.total_ms);
              }
            } else {
              adasdf::QueryEngine engine(model, config, expansion_options);
              const auto setup0 = std::chrono::steady_clock::now();
              if (!engine.prepare()) {
                throw std::runtime_error("QueryEngine prepare failed");
              }
              const auto setup1 = std::chrono::steady_clock::now();
              row.timing.setup_ms =
                  std::chrono::duration<double, std::milli>(setup1 - setup0).count();
              if (engine.stats().setup_ms > 0.0) {
                row.timing.setup_ms = engine.stats().setup_ms;
              }
              row.expanded_memory_mb =
                  static_cast<double>(engine.stats().expanded_memory_bytes) /
                  (1024.0 * 1024.0);
              for (int i = 0; i < warmup; ++i) {
                output = engine.queryBatch(points);
              }
              for (int i = 0; i < repeat; ++i) {
                output = engine.queryBatch(points);
                adasdf::BatchQueryTiming timing = engine.stats().timing;
                if (timing.total_ms <= 0.0) {
                  timing.total_ms = engine.stats().query_total_ms;
                }
                timings.push_back(timing);
                total_values.push_back(timing.total_ms);
              }
              row.fallback_count = engine.stats().fallback_count;
            }
          }

          finishRowFromRuns(row, timings);
          row.kernel_stats = summarize(kernel_values);
          row.total_stats = summarize(total_values);
          if (backend == adasdf::QueryBackend::CUDA) {
            row.query_kernel_ms = row.kernel_stats.mean;
          } else {
            row.query_kernel_ms = 0.0;
          }
          row.query_total_ms = row.total_stats.mean;

          const double reported_ms =
              kernel_only && backend == adasdf::QueryBackend::CUDA
                  ? row.kernel_stats.mean
                  : row.total_stats.mean;
          row.ns_per_query =
              count > 0 ? reported_ms * 1.0e6 / static_cast<double>(count) : 0.0;
          row.queries_per_second =
              reported_ms > 0.0
                  ? static_cast<double>(count) * 1000.0 / reported_ms
                  : 0.0;
          if (download_results) {
            computeErrors(
                reference,
                output,
                adasdf::includesNormals(output_mode),
                row.max_abs_phi_error,
                row.max_normal_error);
            if (expansion != adasdf::QueryExpansionMode::None) {
              const adasdf::ExpansionQualityReport quality =
                  computeQualityFromOutputs(
                      points,
                      reference,
                      output,
                      near_surface_band,
                      sign_epsilon,
                      row.fallback_count);
              row.max_abs_error = quality.max_abs_error;
              row.mean_abs_error = quality.mean_abs_error;
              row.rms_error = quality.rms_error;
              row.p95_abs_error = quality.p95_abs_error;
              row.sign_mismatch_count = quality.sign_mismatch_count;
              row.sign_mismatch_rate = quality.sign_mismatch_rate;
              row.ambiguous_sign_count = quality.ambiguous_sign_count;
              row.ambiguous_sign_rate = quality.ambiguous_sign_rate;
              row.near_surface_sign_mismatch_count =
                  quality.near_surface_sign_mismatch_count;
              row.near_surface_sign_mismatch_rate =
                  quality.near_surface_sign_mismatch_rate;
              row.fallback_rate = quality.fallback_rate;
            }
          } else {
            row.correctness_checked = false;
            row.timing.correctness_checked = false;
          }
          rows.push_back(row);
        } catch (const std::exception& error) {
          row.status = "failed";
          row.error_message = error.what();
          rows.push_back(row);
        }
      }
    }

    writeCsv(std::cout, rows);
    printSummary(rows);

    if (!output_path.empty()) {
      if (!output_path.parent_path().empty()) {
        std::filesystem::create_directories(output_path.parent_path());
      }
      std::ofstream file(output_path);
      if (!file) {
        throw std::runtime_error("failed to open benchmark output CSV");
      }
      writeCsv(file, rows);
    }

    const bool any_failed = std::any_of(
        rows.begin(),
        rows.end(),
        [](const BenchmarkRow& row) { return row.status == "failed"; });
    return any_failed ? 1 : 0;
  } catch (const std::exception& error) {
    std::cerr << "adasdf_benchmark_batch_query failed: " << error.what()
              << "\n";
    return 1;
  }
}
