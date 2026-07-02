#include <adasdf/adasdf.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

enum class Mode {
  PhiOnly,
  PhiNormal
};

struct Metrics {
  std::size_t sample_count = 0;
  std::size_t active_block_count = 0;
  std::size_t expanded_block_count = 0;
  std::size_t cache_memory_bytes = 0;
  double cache_hit_rate = 0.0;
  std::size_t fallback_query_count = 0;
  double active_block_total_ms = 0.0;
  double active_block_avg_ms = 0.0;
  double active_block_ns_per_sample = 0.0;
  double direct_total_ms = 0.0;
  double direct_avg_ms = 0.0;
  double direct_ns_per_sample = 0.0;
};

void usage() {
  std::cout
      << "Usage: adasdf_benchmark_block_cache model.sdfbin samples.csv "
         "[--repeat N] [--warmup N] [--mode phi-only|phi-normal] "
         "[--threshold value] [--selection-band value] [--extra-margin value] "
         "[--no-radius] [--no-neighbors] [--cache-max-blocks N] "
         "[--cache-max-mb value] [--compare-direct] "
         "[--csv benchmark.csv] [--report benchmark.md] [--json benchmark.json]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

Mode parseMode(const std::string& text) {
  if (text == "phi-only") {
    return Mode::PhiOnly;
  }
  if (text == "phi-normal" || text == "phi+normal") {
    return Mode::PhiNormal;
  }
  throw std::runtime_error("unsupported active block benchmark mode: " + text);
}

const char* toString(Mode mode) {
  return mode == Mode::PhiOnly ? "phi-only" : "phi-normal";
}

bool writeText(const std::filesystem::path& path, const std::string& text) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream file(path);
  if (!file) {
    return false;
  }
  file << text;
  return true;
}

std::string metricsCSV(
    const Metrics& metrics,
    int repeat,
    int warmup,
    Mode mode,
    double threshold,
    double selection_band) {
  return
      "sample_count,repeat,warmup,active_block_count,expanded_block_count,"
      "cache_memory_bytes,cache_hit_rate,fallback_query_count,"
      "active_block_avg_ms,active_block_ns_per_sample,direct_avg_ms,"
      "direct_ns_per_sample,mode,threshold,selection_band,status\n" +
      std::to_string(metrics.sample_count) + "," +
      std::to_string(repeat) + "," +
      std::to_string(warmup) + "," +
      std::to_string(metrics.active_block_count) + "," +
      std::to_string(metrics.expanded_block_count) + "," +
      std::to_string(metrics.cache_memory_bytes) + "," +
      std::to_string(metrics.cache_hit_rate) + "," +
      std::to_string(metrics.fallback_query_count) + "," +
      std::to_string(metrics.active_block_avg_ms) + "," +
      std::to_string(metrics.active_block_ns_per_sample) + "," +
      std::to_string(metrics.direct_avg_ms) + "," +
      std::to_string(metrics.direct_ns_per_sample) + "," +
      toString(mode) + "," +
      std::to_string(threshold) + "," +
      std::to_string(selection_band) + ",ok\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1 || (argc > 1 && std::string(argv[1]) == "--help")) {
      usage();
      return 0;
    }
    if (argc < 3) {
      usage();
      return 1;
    }

    const std::filesystem::path model_path = argv[1];
    const std::filesystem::path samples_path = argv[2];
    int repeat = 10;
    int warmup = 1;
    Mode mode = Mode::PhiOnly;
    bool compare_direct = false;
    adasdf::ActiveBlockSelectionOptions selection_options;
    adasdf::ActiveBlockQueryOptions query_options;
    adasdf::ExpandedBlockCacheOptions cache_options;
    std::filesystem::path csv_path;
    std::filesystem::path report_path;
    std::filesystem::path json_path;

    for (int i = 3; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--repeat" && hasValue(i, argc)) {
        repeat = std::stoi(argv[++i]);
      } else if (arg == "--warmup" && hasValue(i, argc)) {
        warmup = std::stoi(argv[++i]);
      } else if (arg == "--mode" && hasValue(i, argc)) {
        mode = parseMode(argv[++i]);
      } else if (arg == "--threshold" && hasValue(i, argc)) {
        selection_options.threshold = std::stod(argv[++i]);
        query_options.threshold = selection_options.threshold;
      } else if (arg == "--selection-band" && hasValue(i, argc)) {
        selection_options.selection_band = std::stod(argv[++i]);
      } else if (arg == "--extra-margin" && hasValue(i, argc)) {
        selection_options.extra_margin = std::stod(argv[++i]);
      } else if (arg == "--no-radius") {
        selection_options.use_sample_radius = false;
        query_options.use_sample_radius = false;
      } else if (arg == "--no-neighbors") {
        selection_options.include_neighbor_blocks = false;
      } else if (arg == "--cache-max-blocks" && hasValue(i, argc)) {
        cache_options.max_blocks =
            static_cast<std::size_t>(std::stoull(argv[++i]));
      } else if (arg == "--cache-max-mb" && hasValue(i, argc)) {
        cache_options.max_memory_bytes = static_cast<std::size_t>(
            std::stod(argv[++i]) * 1024.0 * 1024.0);
      } else if (arg == "--compare-direct") {
        compare_direct = true;
      } else if (arg == "--csv" && hasValue(i, argc)) {
        csv_path = argv[++i];
      } else if (arg == "--report" && hasValue(i, argc)) {
        report_path = argv[++i];
      } else if (arg == "--json" && hasValue(i, argc)) {
        json_path = argv[++i];
      } else if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else {
        std::cerr << "Unknown or incomplete option: " << arg << "\n";
        usage();
        return 1;
      }
    }

    repeat = std::max(1, repeat);
    warmup = std::max(0, warmup);
    query_options.compute_normals = mode == Mode::PhiNormal;
    query_options.output_mode = query_options.compute_normals
        ? adasdf::SparseQueryOutputMode::PhiAndNormal
        : adasdf::SparseQueryOutputMode::PhiOnly;

    const auto model = adasdf::SDFBinReader::read(model_path);
    if (!model || !model->isValid() || !model->queryBackendAvailable()) {
      std::cerr << "adasdf_benchmark_block_cache: failed to load queryable model\n";
      return 1;
    }
    const auto samples =
        adasdf::CollisionSampleSetIO::readCSV(samples_path.string());
    if (!samples.success) {
      std::cerr << "adasdf_benchmark_block_cache: failed to read samples: "
                << samples.error_message << "\n";
      return 1;
    }

    const adasdf::ActiveBlockSelectionResult selection =
        adasdf::ActiveBlockSelector::select(
            *model, samples.sample_set, selection_options);
    if (!selection.success) {
      std::cerr << "adasdf_benchmark_block_cache: selection failed: "
                << selection.error_message << "\n";
      return 2;
    }

    Metrics metrics;
    metrics.sample_count = samples.sample_set.size();
    metrics.active_block_count = selection.block_ids.size();
    adasdf::ExpandedBlockCache cache(cache_options);

    auto run_active_once = [&]() {
      const auto result = adasdf::ActiveBlockQuery::query(
          *model, samples.sample_set, selection.block_ids, cache, query_options);
      if (!result.success) {
        throw std::runtime_error(result.error_message);
      }
      metrics.expanded_block_count =
          result.stats.cache_stats.block_count;
      metrics.cache_memory_bytes =
          result.stats.cache_stats.memory_bytes;
      metrics.cache_hit_rate = result.stats.cache_stats.hitRate();
      metrics.fallback_query_count += result.stats.fallback_query_count;
    };

    for (int i = 0; i < warmup; ++i) {
      run_active_once();
    }
    metrics.fallback_query_count = 0;

    const auto active_start = std::chrono::steady_clock::now();
    for (int i = 0; i < repeat; ++i) {
      run_active_once();
    }
    metrics.active_block_total_ms =
        std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - active_start)
            .count();
    metrics.active_block_avg_ms =
        metrics.active_block_total_ms / static_cast<double>(repeat);
    metrics.active_block_ns_per_sample =
        metrics.sample_count > 0
            ? metrics.active_block_avg_ms * 1.0e6 /
                  static_cast<double>(metrics.sample_count)
            : 0.0;

    if (compare_direct) {
      adasdf::SparseSDFQueryOptions direct_options;
      direct_options.threshold = query_options.threshold;
      direct_options.compute_normals = query_options.compute_normals;
      direct_options.output_mode = query_options.output_mode;
      direct_options.use_sample_radius = query_options.use_sample_radius;
      direct_options.include_non_colliding_samples = true;

      auto run_direct_once = [&]() {
        const auto result =
            adasdf::SparseSDFQuery::query(*model, samples.sample_set, direct_options);
        if (!result.success) {
          throw std::runtime_error(result.error_message);
        }
      };
      for (int i = 0; i < warmup; ++i) {
        run_direct_once();
      }
      const auto direct_start = std::chrono::steady_clock::now();
      for (int i = 0; i < repeat; ++i) {
        run_direct_once();
      }
      metrics.direct_total_ms =
          std::chrono::duration<double, std::milli>(
              std::chrono::steady_clock::now() - direct_start)
              .count();
      metrics.direct_avg_ms =
          metrics.direct_total_ms / static_cast<double>(repeat);
      metrics.direct_ns_per_sample =
          metrics.sample_count > 0
              ? metrics.direct_avg_ms * 1.0e6 /
                    static_cast<double>(metrics.sample_count)
              : 0.0;
    }

    const std::string csv = metricsCSV(
        metrics,
        repeat,
        warmup,
        mode,
        selection_options.threshold,
        selection_options.selection_band);
    std::cout << csv;
    std::cout << "Active block cache benchmark mode: " << toString(mode) << "\n";
    std::cout << "Average ns per sample: "
              << metrics.active_block_ns_per_sample << "\n";
    std::cout << "Status: ok\n";

    if (!csv_path.empty() && !writeText(csv_path, csv)) {
      std::cerr << "adasdf_benchmark_block_cache: failed to write CSV\n";
      return 2;
    }
    if (!report_path.empty()) {
      const std::string md =
          "# Active Block Cache Benchmark\n\n"
          "- Mode: " + std::string(toString(mode)) + "\n"
          "- Sample count: " + std::to_string(metrics.sample_count) + "\n"
          "- Active blocks: " + std::to_string(metrics.active_block_count) + "\n"
          "- Expanded resident blocks: " +
          std::to_string(metrics.expanded_block_count) + "\n"
          "- Cache memory bytes: " +
          std::to_string(metrics.cache_memory_bytes) + "\n"
          "- Active block average ms: " +
          std::to_string(metrics.active_block_avg_ms) + "\n"
          "- Direct sparse average ms: " +
          std::to_string(metrics.direct_avg_ms) + "\n";
      if (!writeText(report_path, md)) {
        std::cerr << "adasdf_benchmark_block_cache: failed to write report\n";
        return 2;
      }
    }
    if (!json_path.empty()) {
      const std::string json =
          "{\n"
          "  \"sample_count\": " + std::to_string(metrics.sample_count) + ",\n"
          "  \"active_block_count\": " +
          std::to_string(metrics.active_block_count) + ",\n"
          "  \"expanded_block_count\": " +
          std::to_string(metrics.expanded_block_count) + ",\n"
          "  \"cache_memory_bytes\": " +
          std::to_string(metrics.cache_memory_bytes) + ",\n"
          "  \"active_block_avg_ms\": " +
          std::to_string(metrics.active_block_avg_ms) + ",\n"
          "  \"direct_avg_ms\": " + std::to_string(metrics.direct_avg_ms) + ",\n"
          "  \"mode\": \"" + std::string(toString(mode)) + "\"\n"
          "}\n";
      if (!writeText(json_path, json)) {
        std::cerr << "adasdf_benchmark_block_cache: failed to write JSON report\n";
        return 2;
      }
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_benchmark_block_cache failed: " << exc.what() << "\n";
    return 2;
  }
}
