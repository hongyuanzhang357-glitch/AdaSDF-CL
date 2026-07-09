#include <adasdf/adasdf.h>

#include "BuildCliProfileHelpers.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct BenchmarkOptions {
  std::filesystem::path input;
  std::string builder = "compressed";
  std::vector<std::string> cache_modes{"off", "block"};
  std::filesystem::path csv;
  std::filesystem::path report;
  std::string case_id = "build_cache";
  adasdf::AdaptiveBlockSDFBuildOptions build_options;
  adasdf::BlockLowRankCompressionOptions compression_options;
};

struct QualityDiff {
  double max_abs_phi_diff = 0.0;
  double rms_phi_diff = 0.0;
  double p95_phi_diff = 0.0;
  std::size_t sign_mismatch_count = 0;
  bool comparable = false;
};

struct RunResult {
  std::string cache_mode;
  bool success = false;
  std::string error_message;
  double time_ms = 0.0;
  adasdf::AdaptiveBlockSDFBuildReport build_report;
  adasdf::BlockLowRankCompressionReport compression_report;
  std::shared_ptr<adasdf::AdaptiveBlockSDFModel> adaptive;
  QualityDiff diff;
};

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

std::vector<std::string> splitCsv(const std::string& text) {
  std::vector<std::string> values;
  std::string current;
  std::istringstream in(text);
  while (std::getline(in, current, ',')) {
    if (!current.empty()) {
      values.push_back(current);
    }
  }
  return values;
}

bool parseSamplingMode(
    const std::string& value,
    adasdf::AdaptiveBlockSDFBuildOptions* options) {
  if (value == "exact") {
    options->hierarchical_sampling.enable_hierarchical_sampling = false;
    options->contact_band_sampling.enable_contact_band_sampling = false;
    return true;
  }
  if (value == "hierarchical") {
    options->hierarchical_sampling.enable_hierarchical_sampling = true;
    options->contact_band_sampling.enable_contact_band_sampling = false;
    return true;
  }
  if (value == "contact-band") {
    options->hierarchical_sampling.enable_hierarchical_sampling = false;
    options->contact_band_sampling.enable_contact_band_sampling = true;
    return true;
  }
  return false;
}

bool cacheScopeFromMode(
    const std::string& mode,
    adasdf::BuildCacheScope* scope) {
  return adasdf::parseBuildCacheScope(mode, scope);
}

void configureCache(
    const std::string& mode,
    adasdf::AdaptiveBlockSDFBuildOptions* options) {
  adasdf::BuildCacheScope scope = adasdf::BuildCacheScope::Off;
  cacheScopeFromMode(mode, &scope);
  options->cache_options.sample_cache = scope;
  options->cache_options.corner_cache = scope;
  options->cache_options.sign_cache = scope != adasdf::BuildCacheScope::Off;
  options->cache_options.distance_cache =
      scope != adasdf::BuildCacheScope::Off;
  options->cache_options.marker_cache =
      scope != adasdf::BuildCacheScope::Off &&
      options->contact_band_sampling.enable_contact_band_sampling;
  options->cache_options.report_cache_stats = true;
}

std::vector<double> allPhi(const adasdf::AdaptiveBlockSDFModel& model) {
  std::vector<double> values;
  for (const adasdf::AdaptiveSDFBlock& block : model.blockSet().blocks) {
    values.insert(values.end(), block.phi.begin(), block.phi.end());
  }
  return values;
}

QualityDiff compareModels(
    const adasdf::AdaptiveBlockSDFModel& reference,
    const adasdf::AdaptiveBlockSDFModel& candidate) {
  QualityDiff diff;
  if (reference.blockSet().blocks.size() != candidate.blockSet().blocks.size()) {
    return diff;
  }
  const std::vector<double> a = allPhi(reference);
  const std::vector<double> b = allPhi(candidate);
  if (a.size() != b.size()) {
    return diff;
  }
  diff.comparable = true;
  if (a.empty()) {
    return diff;
  }
  std::vector<double> abs_diffs;
  abs_diffs.reserve(a.size());
  double sum_sq = 0.0;
  for (std::size_t i = 0; i < a.size(); ++i) {
    const double d = std::abs(a[i] - b[i]);
    abs_diffs.push_back(d);
    diff.max_abs_phi_diff = std::max(diff.max_abs_phi_diff, d);
    sum_sq += d * d;
    const int sa = a[i] < 0.0 ? -1 : (a[i] > 0.0 ? 1 : 0);
    const int sb = b[i] < 0.0 ? -1 : (b[i] > 0.0 ? 1 : 0);
    if (sa != sb) {
      ++diff.sign_mismatch_count;
    }
  }
  diff.rms_phi_diff = std::sqrt(sum_sq / static_cast<double>(a.size()));
  std::sort(abs_diffs.begin(), abs_diffs.end());
  const std::size_t p95_index =
      std::min(abs_diffs.size() - 1,
               static_cast<std::size_t>(
                   std::floor(0.95 * static_cast<double>(abs_diffs.size() - 1))));
  diff.p95_phi_diff = abs_diffs[p95_index];
  return diff;
}

RunResult runOne(const BenchmarkOptions& base, const std::string& mode) {
  RunResult result;
  result.cache_mode = mode;
  adasdf::AdaptiveBlockSDFBuildOptions build_options = base.build_options;
  configureCache(mode, &build_options);
  const auto t0 = std::chrono::steady_clock::now();
  std::shared_ptr<adasdf::SDFModel> model = adasdf::AdaptiveBlockSDFBuilder::fromSTL(
      base.input.string(),
      build_options,
      &result.build_report);
  if (!model) {
    result.error_message = result.build_report.error_message;
    return result;
  }
  result.adaptive =
      std::dynamic_pointer_cast<adasdf::AdaptiveBlockSDFModel>(model);
  if (!result.adaptive) {
    result.error_message = "adaptive builder returned unexpected model type";
    return result;
  }
  if (base.builder == "compressed") {
    adasdf::CompressedAdaptiveBlockSDF compressed =
        adasdf::BlockLowRankCompressor::compress(
            result.adaptive->blockSet(),
            base.compression_options,
            &result.compression_report);
    if (!result.compression_report.success) {
      result.error_message = result.compression_report.error_message;
      return result;
    }
    (void)compressed;
  }
  const auto t1 = std::chrono::steady_clock::now();
  result.time_ms =
      std::chrono::duration<double, std::milli>(t1 - t0).count();
  result.success = true;
  return result;
}

double hitRate(std::size_t hits, std::size_t misses) {
  const std::size_t total = hits + misses;
  return total > 0 ? static_cast<double>(hits) / static_cast<double>(total)
                   : 0.0;
}

std::string csvHeader() {
  return "case_id,builder,sampling,cache_mode,no_cache_time_ms,cache_time_ms,"
         "speedup_vs_no_cache,num_distance_queries_no_cache,"
         "num_distance_queries_cache,num_sign_queries_no_cache,"
         "num_sign_queries_cache,num_triangle_tests_no_cache,"
         "num_triangle_tests_cache,sample_cache_hit_rate,"
         "corner_cache_hit_rate,sign_cache_hit_rate,distance_queries_saved,"
         "sign_queries_saved,duplicate_point_count,marker_cache_hit_rate,"
         "max_abs_phi_diff,rms_phi_diff,p95_phi_diff,sign_mismatch_count,"
         "normal_p95_error_deg,quality_passed,performance_claim_allowed";
}

std::string csvRow(
    const BenchmarkOptions& options,
    const RunResult& reference,
    const RunResult& result) {
  const adasdf::BuildCacheStats& cache = result.build_report.cache_stats;
  const adasdf::BuildCacheStats& ref_cache = reference.build_report.cache_stats;
  const std::size_t ref_distance =
      reference.build_report.contact_band_sampling.distance_query_count > 0
          ? reference.build_report.contact_band_sampling.distance_query_count
          : reference.build_report.acceleration_stats.sample_count;
  const std::size_t result_distance =
      result.build_report.contact_band_sampling.distance_query_count > 0
          ? result.build_report.contact_band_sampling.distance_query_count
          : result.build_report.acceleration_stats.sample_count;
  const std::size_t ref_sign =
      reference.build_report.contact_band_sampling.sign_query_count > 0
          ? reference.build_report.contact_band_sampling.sign_query_count
          : ref_distance;
  const std::size_t result_sign =
      result.build_report.contact_band_sampling.sign_query_count > 0
          ? result.build_report.contact_band_sampling.sign_query_count
          : result_distance;
  const std::size_t ref_tri =
      reference.build_report.acceleration_stats.nearest_triangle_tests +
      reference.build_report.acceleration_stats.ray_triangle_tests;
  const std::size_t result_tri =
      result.build_report.acceleration_stats.nearest_triangle_tests +
      result.build_report.acceleration_stats.ray_triangle_tests;
  const bool quality_passed =
      result.diff.comparable && result.diff.max_abs_phi_diff <= 1e-12 &&
      result.diff.sign_mismatch_count == 0;
  const double speedup =
      result.time_ms > 0.0 ? reference.time_ms / result.time_ms : 0.0;
  const bool performance_claim_allowed = speedup > 1.0 && quality_passed;
  std::ostringstream out;
  out << options.case_id << "," << options.builder << ","
      << (options.build_options.contact_band_sampling.enable_contact_band_sampling
              ? "contact-band"
              : (options.build_options.hierarchical_sampling
                         .enable_hierarchical_sampling
                     ? "hierarchical"
                     : "exact"))
      << "," << result.cache_mode << "," << reference.time_ms << ","
      << result.time_ms << "," << speedup << "," << ref_distance << ","
      << result_distance << "," << ref_sign << "," << result_sign << ","
      << ref_tri << "," << result_tri << ","
      << cache.sample_cache_hit_rate << ","
      << hitRate(cache.corner_cache_hits, cache.corner_cache_misses) << ","
      << hitRate(cache.sign_cache_hits, cache.sign_cache_misses) << ","
      << cache.distance_queries_saved << "," << cache.sign_queries_saved
      << "," << cache.block_point_duplicate_count << ","
      << hitRate(cache.marker_decision_cache_hits,
                 cache.marker_decision_cache_misses)
      << "," << result.diff.max_abs_phi_diff << ","
      << result.diff.rms_phi_diff << "," << result.diff.p95_phi_diff << ","
      << result.diff.sign_mismatch_count << ",0,"
      << (quality_passed ? "true" : "false") << ","
      << (performance_claim_allowed ? "true" : "false");
  (void)ref_cache;
  return out.str();
}

void writeCsv(
    const std::filesystem::path& path,
    const std::vector<std::string>& rows) {
  if (path.empty()) {
    return;
  }
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream out(path);
  out << csvHeader() << "\n";
  for (const std::string& row : rows) {
    out << row << "\n";
  }
}

void writeMarkdown(
    const std::filesystem::path& path,
    const BenchmarkOptions& options,
    const std::vector<RunResult>& results,
    const std::vector<std::string>& rows) {
  if (path.empty()) {
    return;
  }
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream out(path);
  out << "# Build Cache Benchmark\n\n";
  out << "- Case id: " << options.case_id << "\n";
  out << "- Builder: " << options.builder << "\n";
  out << "- Input: " << options.input.string() << "\n\n";
  out << "```csv\n" << csvHeader() << "\n";
  for (const std::string& row : rows) {
    out << row << "\n";
  }
  out << "```\n\n";
  for (const RunResult& result : results) {
    out << "## " << result.cache_mode << "\n\n";
    out << adasdf::BuildCacheReportWriter::toMarkdown(
        result.build_report.cache_stats);
    out << "\n";
  }
}

void usage() {
  std::cout
      << "Usage: adasdf_benchmark_build_cache input.stl "
         "[--builder adaptive|compressed] [--max-level N] "
         "[--min-level N] [--block-resolution N] [--target-error value] "
         "[--max-rank N] [--sampling exact|hierarchical|contact-band] "
         "[--contact-band-width value] "
         "[--contact-band-marker conservative-aabb|distance-aware|hybrid] "
         "[--marker-cell-size-factor value] [--marker-safety-factor value] "
         "[--local-halo-only] [--cache-modes off,block,global] "
         "[--distance-backend brute_force|brute|bvh] [--threads auto|N] "
         "[--csv out.csv] [--report out.md] [--case-id id]\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1) {
      usage();
      return 0;
    }
    BenchmarkOptions options;
    options.build_options.require_watertight_for_signed = true;
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else if (arg == "--builder" && hasValue(i, argc)) {
        options.builder = argv[++i];
        if (options.builder != "adaptive" && options.builder != "compressed") {
          std::cerr << "Unknown builder: " << options.builder << "\n";
          return 1;
        }
      } else if (arg == "--max-level" && hasValue(i, argc)) {
        options.build_options.max_octree_level = std::stoi(argv[++i]);
      } else if (arg == "--min-level" && hasValue(i, argc)) {
        options.build_options.min_octree_level = std::stoi(argv[++i]);
      } else if (arg == "--block-resolution" && hasValue(i, argc)) {
        options.build_options.block_resolution = std::stoi(argv[++i]);
      } else if (arg == "--target-error" && hasValue(i, argc)) {
        const double value = std::stod(argv[++i]);
        options.build_options.target_near_surface_error = value;
        options.compression_options.target_max_abs_error = value;
        options.compression_options.target_p95_error = value;
      } else if (arg == "--max-rank" && hasValue(i, argc)) {
        options.compression_options.max_rank = std::stoi(argv[++i]);
      } else if (arg == "--sampling" && hasValue(i, argc)) {
        if (!parseSamplingMode(argv[++i], &options.build_options)) {
          std::cerr << "Unknown sampling mode: " << argv[i] << "\n";
          return 1;
        }
      } else if (arg == "--contact-band-width" && hasValue(i, argc)) {
        options.build_options.contact_band_sampling.contact_band_width =
            std::stod(argv[++i]);
      } else if (arg == "--contact-band-marker" && hasValue(i, argc)) {
        adasdf::ContactBandMarkerMode mode;
        if (!adasdf::parseContactBandMarkerMode(argv[++i], &mode)) {
          std::cerr << "Unknown contact-band marker: " << argv[i] << "\n";
          return 1;
        }
        options.build_options.contact_band_sampling.marker_mode = mode;
      } else if (arg == "--marker-cell-size-factor" && hasValue(i, argc)) {
        options.build_options.contact_band_sampling.marker_cell_size_factor =
            std::stod(argv[++i]);
      } else if (arg == "--marker-safety-factor" && hasValue(i, argc)) {
        options.build_options.contact_band_sampling.marker_safety_factor =
            std::stod(argv[++i]);
      } else if (arg == "--local-halo-only") {
        options.build_options.contact_band_sampling.local_halo_only = true;
        options.build_options.contact_band_sampling.disable_global_halo = true;
      } else if (arg == "--cache-modes" && hasValue(i, argc)) {
        options.cache_modes = splitCsv(argv[++i]);
      } else if ((arg == "--distance-backend" || arg == "--accel") &&
                 hasValue(i, argc)) {
        adasdf::SDFSamplingAcceleration acceleration;
        if (!adasdf_tools::parseAccelerationAlias(argv[++i], &acceleration)) {
          std::cerr << "Unknown distance backend: " << argv[i] << "\n";
          return 1;
        }
        options.build_options.acceleration = acceleration;
      } else if (arg == "--threads" && hasValue(i, argc)) {
        options.build_options.threads = adasdf_tools::parseThreadsAuto(argv[++i]);
      } else if (arg == "--csv" && hasValue(i, argc)) {
        options.csv = argv[++i];
      } else if (arg == "--report" && hasValue(i, argc)) {
        options.report = argv[++i];
      } else if (arg == "--case-id" && hasValue(i, argc)) {
        options.case_id = argv[++i];
      } else if (!arg.empty() && arg[0] == '-') {
        std::cerr << "Unknown or incomplete option: " << arg << "\n";
        usage();
        return 1;
      } else if (options.input.empty()) {
        options.input = arg;
      } else {
        std::cerr << "Unexpected positional argument: " << arg << "\n";
        usage();
        return 1;
      }
    }
    if (options.input.empty()) {
      usage();
      return 1;
    }
    if (std::find(options.cache_modes.begin(), options.cache_modes.end(), "off") ==
        options.cache_modes.end()) {
      options.cache_modes.insert(options.cache_modes.begin(), "off");
    }
    std::vector<RunResult> results;
    for (const std::string& mode : options.cache_modes) {
      adasdf::BuildCacheScope scope;
      if (!cacheScopeFromMode(mode, &scope)) {
        std::cerr << "Unknown cache mode: " << mode << "\n";
        return 1;
      }
      RunResult result = runOne(options, mode);
      if (!result.success) {
        std::cerr << "Build cache benchmark failed for mode " << mode << ": "
                  << result.error_message << "\n";
        return 2;
      }
      results.push_back(std::move(result));
    }
    const RunResult* reference = nullptr;
    for (const RunResult& result : results) {
      if (result.cache_mode == "off") {
        reference = &result;
        break;
      }
    }
    if (reference == nullptr || !reference->adaptive) {
      std::cerr << "No no-cache reference result was produced.\n";
      return 3;
    }
    std::vector<std::string> rows;
    for (RunResult& result : results) {
      result.diff = compareModels(*reference->adaptive, *result.adaptive);
      rows.push_back(csvRow(options, *reference, result));
    }
    writeCsv(options.csv, rows);
    writeMarkdown(options.report, options, results, rows);
    std::cout << "AdaSDF-CL build cache benchmark\n";
    std::cout << "Case id: " << options.case_id << "\n";
    std::cout << "Builder: " << options.builder << "\n";
    std::cout << "Reference cache mode: off\n";
    std::cout << csvHeader() << "\n";
    for (const std::string& row : rows) {
      std::cout << row << "\n";
    }
    if (!options.csv.empty()) {
      std::cout << "CSV: " << options.csv.string() << "\n";
    }
    if (!options.report.empty()) {
      std::cout << "Report: " << options.report.string() << "\n";
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_benchmark_build_cache failed: " << exc.what() << "\n";
    return 3;
  }
}
