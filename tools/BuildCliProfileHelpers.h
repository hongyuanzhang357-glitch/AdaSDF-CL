#pragma once

#include <adasdf/adasdf.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

namespace adasdf_tools {

struct BuildCliRuntimeOptions {
  bool profile_stderr = false;
  bool progress_stderr = false;
  std::filesystem::path profile_json_path;
  std::filesystem::path progress_json_path;
  double max_seconds = 0.0;
};

enum class CommonOptionParseResult {
  NotCommon,
  Parsed,
  Error,
};

inline bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

inline int parseThreadsAuto(const std::string& value) {
  if (value == "auto") {
    const unsigned int hardware = std::thread::hardware_concurrency();
    return static_cast<int>(hardware == 0 ? 1 : hardware);
  }
  return std::stoi(value);
}

inline bool parseAccelerationAlias(
    const std::string& value,
    adasdf::SDFSamplingAcceleration* acceleration) {
  if (value == "brute_force") {
    *acceleration = adasdf::SDFSamplingAcceleration::BruteForce;
    return true;
  }
  return adasdf::parseSDFSamplingAcceleration(value, acceleration);
}

inline CommonOptionParseResult parseBuildCliRuntimeOption(
    const std::string& arg,
    int* index,
    int argc,
    char** argv,
    BuildCliRuntimeOptions* options) {
  if (arg == "--profile") {
    options->profile_stderr = true;
    return CommonOptionParseResult::Parsed;
  }
  if (arg == "--profile-json") {
    if (!hasValue(*index, argc)) {
      std::cerr << "Missing value for --profile-json\n";
      return CommonOptionParseResult::Error;
    }
    options->profile_json_path = argv[++(*index)];
    return CommonOptionParseResult::Parsed;
  }
  if (arg == "--progress") {
    options->progress_stderr = true;
    return CommonOptionParseResult::Parsed;
  }
  if (arg == "--progress-json") {
    if (!hasValue(*index, argc)) {
      std::cerr << "Missing value for --progress-json\n";
      return CommonOptionParseResult::Error;
    }
    options->progress_json_path = argv[++(*index)];
    return CommonOptionParseResult::Parsed;
  }
  if (arg == "--max-seconds") {
    if (!hasValue(*index, argc)) {
      std::cerr << "Missing value for --max-seconds\n";
      return CommonOptionParseResult::Error;
    }
    options->max_seconds = std::stod(argv[++(*index)]);
    return CommonOptionParseResult::Parsed;
  }
  return CommonOptionParseResult::NotCommon;
}

inline bool profileRequested(const BuildCliRuntimeOptions& options) {
  return options.profile_stderr || !options.profile_json_path.empty();
}

inline void writeBuildProfile(
    const std::string& tool_name,
    const BuildCliRuntimeOptions& options,
    const adasdf::BuildProfiler& profiler) {
  if (!options.profile_json_path.empty() &&
      !adasdf::BuildProfileJsonWriter::write(
          options.profile_json_path.string(),
          profiler)) {
    std::cerr << tool_name << ": failed to write profile JSON\n";
  }
  if (options.profile_stderr) {
    std::cerr << adasdf::BuildProfileJsonWriter::toJson(profiler);
  }
}

inline int timeoutExit(
    const std::string& tool_name,
    const BuildCliRuntimeOptions& options,
    adasdf::BuildProfiler* profiler,
    const adasdf::ProgressReporter& progress) {
  progress.emit("timeout", "maximum wall-clock time reached");
  profiler->finishTimeout();
  writeBuildProfile(tool_name, options, *profiler);
  return adasdf::toInt(adasdf::CliExitCode::Timeout);
}

inline void addWarnings(
    adasdf::BuildProfiler* profiler,
    const std::string& code,
    const std::vector<std::string>& warnings) {
  for (const std::string& warning : warnings) {
    profiler->addWarning(code, warning);
  }
}

inline void fillFromAccelerationStats(
    adasdf::BuildProfiler* profiler,
    const adasdf::BuildAccelerationStats& stats) {
  profiler->timings.bvh_build_time_ms = stats.bvh_build_time_ms;
  profiler->timings.distance_query_time_ms = stats.sampling_time_ms;
  profiler->counters.num_grid_points = stats.sample_count;
  profiler->counters.num_distance_queries = stats.sample_count;
  profiler->counters.num_sign_queries = stats.sample_count;
  profiler->counters.num_triangle_tests_total =
      stats.nearest_triangle_tests + stats.ray_triangle_tests;
  profiler->counters.bvh_node_visit_count =
      stats.nearest_node_visits + stats.ray_node_visits;
  profiler->counters.fallback_count = stats.fallback_count;
  profiler->counters.avg_triangles_tested_per_query =
      stats.sample_count > 0
          ? static_cast<double>(profiler->counters.num_triangle_tests_total) /
                static_cast<double>(stats.sample_count)
          : 0.0;
}

inline void fillDenseProfile(
    adasdf::BuildProfiler* profiler,
    const adasdf::DenseSDFBuildReport& report) {
  fillFromAccelerationStats(profiler, report.acceleration_stats);
  if (profiler->timings.distance_query_time_ms <= 0.0) {
    profiler->timings.distance_query_time_ms = report.build_time_ms;
  }
  profiler->counters.num_vertices = report.vertex_count;
  profiler->counters.num_triangles = report.triangle_count;
  profiler->counters.num_grid_points =
      static_cast<std::size_t>(report.nx) *
      static_cast<std::size_t>(report.ny) *
      static_cast<std::size_t>(report.nz);
  profiler->counters.num_distance_queries = profiler->counters.num_grid_points;
  profiler->counters.num_sign_queries = profiler->counters.num_grid_points;
  addWarnings(profiler, "ADASDF_BUILD_WARNING", report.warnings);
}

inline void fillAdaptiveProfile(
    adasdf::BuildProfiler* profiler,
    const adasdf::AdaptiveBlockSDFBuildReport& report) {
  fillFromAccelerationStats(profiler, report.acceleration_stats);
  if (profiler->timings.distance_query_time_ms <= 0.0) {
    profiler->timings.distance_query_time_ms = report.sampling_time_ms;
  }
  profiler->timings.adaptive_refinement_time_ms = report.build_time_ms;
  profiler->timings.contact_band_marker_time_ms =
      report.contact_band_sampling.contact_band_marker_time_ms;
  profiler->counters.num_vertices = report.diagnostics.vertex_count;
  profiler->counters.num_triangles = report.diagnostics.triangle_count;
  profiler->counters.num_blocks = report.block_count;
  profiler->counters.num_contact_band_blocks =
      report.contact_band_sampling.contact_band_block_count;
  if (report.contact_band_sampling.distance_query_count > 0) {
    profiler->counters.num_distance_queries =
        report.contact_band_sampling.distance_query_count;
  }
  if (report.contact_band_sampling.sign_query_count > 0) {
    profiler->counters.num_sign_queries =
        report.contact_band_sampling.sign_query_count;
  }
  profiler->counters.exact_node_count =
      report.contact_band_sampling.exact_node_count;
  profiler->counters.predicted_node_count =
      report.contact_band_sampling.predicted_node_count;
  profiler->counters.fallback_count +=
      report.hierarchical_sampling.fallback_exact_block_count;
  addWarnings(profiler, "ADASDF_BUILD_WARNING", report.warnings);
}

inline void fillCompressionProfile(
    adasdf::BuildProfiler* profiler,
    const adasdf::BlockLowRankCompressionReport& report) {
  profiler->timings.compression_time_ms = report.compression_time_ms;
  profiler->counters.num_blocks = report.input_block_count;
  profiler->counters.num_contact_band_blocks = report.near_surface_block_count;
  profiler->counters.compressed_block_count = report.compressed_block_count;
  profiler->counters.dense_fallback_block_count =
      report.dense_fallback_block_count;
  addWarnings(profiler, "ADASDF_COMPRESSION_WARNING", report.warnings);
}

}  // namespace adasdf_tools
