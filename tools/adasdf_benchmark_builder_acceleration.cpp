#include <adasdf/adasdf.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_benchmark_builder_acceleration input.stl "
         "[--builder dense|adaptive|compressed] [--accel brute|bvh] "
         "[--threads N] [--repeat N] [--warmup N] "
         "[--benchmark-brute-reference] [--resolution N] "
         "[--min-level N] [--max-level N] [--block-resolution N] "
         "[--unsigned]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

struct Options {
  std::filesystem::path input;
  std::string builder = "dense";
  adasdf::SDFSamplingAcceleration acceleration =
      adasdf::SDFSamplingAcceleration::BVH;
  int threads = 1;
  int repeat = 3;
  int warmup = 1;
  bool benchmark_brute_reference = false;
  bool signed_distance = true;
  int resolution = 16;
  int min_level = 1;
  int max_level = 2;
  int block_resolution = 4;
};

struct RunStats {
  bool success = false;
  adasdf::BuildAccelerationStats acceleration_stats;
  double total_build_time_ms = 0.0;
};

bool parseAcceleration(const std::string& text, Options* options) {
  adasdf::SDFSamplingAcceleration acceleration;
  if (!adasdf::parseSDFSamplingAcceleration(text, &acceleration)) {
    return false;
  }
  options->acceleration = acceleration;
  return true;
}

RunStats runDense(const Options& options) {
  adasdf::DenseSDFBuildOptions build_options;
  build_options.resolution = options.resolution;
  build_options.signed_distance = options.signed_distance;
  build_options.require_watertight_for_signed = options.signed_distance;
  build_options.acceleration = options.acceleration;
  build_options.threads = options.threads;
  build_options.benchmark_brute_reference =
      options.benchmark_brute_reference;
  build_options.verbose = false;
  adasdf::DenseSDFBuildReport report;
  auto model = adasdf::DenseSDFBuilder::fromSTL(
      options.input.string(),
      build_options,
      &report);
  RunStats stats;
  stats.success = static_cast<bool>(model) && report.success;
  stats.acceleration_stats = report.acceleration_stats;
  stats.total_build_time_ms = report.build_time_ms;
  return stats;
}

RunStats runAdaptive(const Options& options) {
  adasdf::AdaptiveBlockSDFBuildOptions build_options;
  build_options.min_octree_level = options.min_level;
  build_options.max_octree_level = options.max_level;
  build_options.block_resolution = options.block_resolution;
  build_options.signed_distance = options.signed_distance;
  build_options.require_watertight_for_signed = options.signed_distance;
  build_options.acceleration = options.acceleration;
  build_options.threads = options.threads;
  build_options.benchmark_brute_reference =
      options.benchmark_brute_reference;
  build_options.verbose = false;
  adasdf::AdaptiveBlockSDFBuildReport report;
  auto model = adasdf::AdaptiveBlockSDFBuilder::fromSTL(
      options.input.string(),
      build_options,
      &report);
  RunStats stats;
  stats.success = static_cast<bool>(model) && report.success;
  stats.acceleration_stats = report.acceleration_stats;
  stats.total_build_time_ms = report.build_time_ms;
  return stats;
}

RunStats runCompressed(const Options& options) {
  adasdf::AdaptiveBlockSDFBuildOptions build_options;
  build_options.min_octree_level = options.min_level;
  build_options.max_octree_level = options.max_level;
  build_options.block_resolution = options.block_resolution;
  build_options.signed_distance = options.signed_distance;
  build_options.require_watertight_for_signed = options.signed_distance;
  build_options.acceleration = options.acceleration;
  build_options.threads = options.threads;
  build_options.benchmark_brute_reference =
      options.benchmark_brute_reference;
  build_options.verbose = false;
  adasdf::AdaptiveBlockSDFBuildReport build_report;
  auto model = adasdf::AdaptiveBlockSDFBuilder::fromSTL(
      options.input.string(),
      build_options,
      &build_report);
  RunStats stats;
  stats.success = static_cast<bool>(model) && build_report.success;
  stats.acceleration_stats = build_report.acceleration_stats;
  stats.total_build_time_ms = build_report.build_time_ms;
  if (!stats.success) {
    return stats;
  }
  auto adaptive =
      std::dynamic_pointer_cast<adasdf::AdaptiveBlockSDFModel>(model);
  if (!adaptive) {
    stats.success = false;
    return stats;
  }
  adasdf::BlockLowRankCompressionReport compression_report;
  adasdf::BlockLowRankCompressionOptions compression_options;
  const auto t0 = std::chrono::steady_clock::now();
  const adasdf::CompressedAdaptiveBlockSDF compressed =
      adasdf::BlockLowRankCompressor::compress(
          adaptive->blockSet(),
          compression_options,
          &compression_report);
  const auto t1 = std::chrono::steady_clock::now();
  (void)compressed;
  stats.success = compression_report.success;
  stats.total_build_time_ms +=
      std::chrono::duration<double, std::milli>(t1 - t0).count();
  return stats;
}

RunStats runOnce(const Options& options) {
  if (options.builder == "dense") {
    return runDense(options);
  }
  if (options.builder == "adaptive") {
    return runAdaptive(options);
  }
  if (options.builder == "compressed") {
    return runCompressed(options);
  }
  return {};
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1) {
      usage();
      return 0;
    }
    Options options;
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else if (arg == "--builder" && hasValue(i, argc)) {
        options.builder = argv[++i];
        if (options.builder != "dense" && options.builder != "adaptive" &&
            options.builder != "compressed") {
          std::cerr << "Unknown builder: " << options.builder << "\n";
          return 1;
        }
      } else if (arg == "--accel" && hasValue(i, argc)) {
        if (!parseAcceleration(argv[++i], &options)) {
          std::cerr << "Unknown acceleration mode: " << argv[i] << "\n";
          return 1;
        }
      } else if (arg == "--threads" && hasValue(i, argc)) {
        options.threads = std::stoi(argv[++i]);
      } else if (arg == "--repeat" && hasValue(i, argc)) {
        options.repeat = std::stoi(argv[++i]);
      } else if (arg == "--warmup" && hasValue(i, argc)) {
        options.warmup = std::stoi(argv[++i]);
      } else if (arg == "--benchmark-brute-reference") {
        options.benchmark_brute_reference = true;
      } else if (arg == "--resolution" && hasValue(i, argc)) {
        options.resolution = std::stoi(argv[++i]);
      } else if (arg == "--min-level" && hasValue(i, argc)) {
        options.min_level = std::stoi(argv[++i]);
      } else if (arg == "--max-level" && hasValue(i, argc)) {
        options.max_level = std::stoi(argv[++i]);
      } else if (arg == "--block-resolution" && hasValue(i, argc)) {
        options.block_resolution = std::stoi(argv[++i]);
      } else if (arg == "--unsigned") {
        options.signed_distance = false;
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
    if (!std::filesystem::exists(options.input)) {
      std::cerr
          << "adasdf_benchmark_builder_acceleration: input STL does not exist: "
          << options.input.string() << "\n";
      return 1;
    }
    options.threads = std::max(1, options.threads);
    options.repeat = std::max(1, options.repeat);
    options.warmup = std::max(0, options.warmup);

    for (int i = 0; i < options.warmup; ++i) {
      const RunStats warmup = runOnce(options);
      if (!warmup.success) {
        std::cerr << "Warmup build failed.\n";
        return 3;
      }
    }

    RunStats last;
    double total_build = 0.0;
    double sampling = 0.0;
    double bvh_build = 0.0;
    double brute_reference = 0.0;
    for (int i = 0; i < options.repeat; ++i) {
      last = runOnce(options);
      if (!last.success) {
        std::cerr << "Benchmark build failed.\n";
        return 3;
      }
      total_build += last.total_build_time_ms;
      sampling += last.acceleration_stats.sampling_time_ms;
      bvh_build += last.acceleration_stats.bvh_build_time_ms;
      brute_reference += last.acceleration_stats.brute_reference_time_ms;
    }
    const double inv_repeat = 1.0 / static_cast<double>(options.repeat);
    const double total_build_avg = total_build * inv_repeat;
    const double sampling_avg = sampling * inv_repeat;
    const double bvh_build_avg = bvh_build * inv_repeat;
    const double brute_reference_avg = brute_reference * inv_repeat;
    const double speedup =
        sampling_avg > 0.0 ? brute_reference_avg / sampling_avg : 0.0;

    std::cout << "AdaSDF-CL builder acceleration benchmark\n";
    std::cout << "Builder: " << options.builder << "\n";
    std::cout << "Acceleration: " << adasdf::toString(options.acceleration)
              << "\n";
    std::cout << "Threads: " << last.acceleration_stats.threads_used << "\n";
    std::cout << "Repeat: " << options.repeat << "\n";
    std::cout << "Warmup: " << options.warmup << "\n";
    std::cout << "Sample count: " << last.acceleration_stats.sample_count
              << "\n";
    std::cout << "BVH nodes: " << last.acceleration_stats.bvh_node_count
              << "\n";
    std::cout << "BVH leaves: " << last.acceleration_stats.bvh_leaf_count
              << "\n";
    std::cout << "BVH build time ms: " << bvh_build_avg << "\n";
    std::cout << "Sampling time ms: " << sampling_avg << "\n";
    std::cout << "Total build time ms: " << total_build_avg << "\n";
    std::cout << "Brute reference time ms: " << brute_reference_avg << "\n";
    std::cout << "Speedup vs brute reference: " << speedup << "\n";
    std::cout
        << "builder,acceleration,threads,sample_count,bvh_build_time_ms,"
           "sampling_time_ms,total_build_time_ms,brute_reference_time_ms,"
           "speedup_vs_bruteforce\n";
    std::cout << options.builder << "," << adasdf::toString(options.acceleration)
              << "," << last.acceleration_stats.threads_used << ","
              << last.acceleration_stats.sample_count << "," << bvh_build_avg
              << "," << sampling_avg << "," << total_build_avg << ","
              << brute_reference_avg << "," << speedup << "\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_benchmark_builder_acceleration failed: "
              << exc.what() << "\n";
    return 3;
  }
}
