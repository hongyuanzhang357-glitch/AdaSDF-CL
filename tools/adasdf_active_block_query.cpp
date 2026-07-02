#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_active_block_query model.sdfbin samples.csv "
         "[--threshold value] [--selection-band value] [--extra-margin value] "
         "[--phi-only] [--with-normal] [--early-exit] [--no-radius] "
         "[--no-neighbors] [--no-fallback] [--colliding-only] [--sort] "
         "[--cache-max-blocks N] [--cache-max-mb value] "
         "[--out results.csv] [--report report.md] [--json report.json]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
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
    adasdf::ActiveBlockSelectionOptions selection_options;
    adasdf::ActiveBlockQueryOptions query_options;
    adasdf::ExpandedBlockCacheOptions cache_options;
    std::filesystem::path out_path;
    std::filesystem::path report_path;
    std::filesystem::path json_path;

    for (int i = 3; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--threshold" && hasValue(i, argc)) {
        selection_options.threshold = std::stod(argv[++i]);
        query_options.threshold = selection_options.threshold;
      } else if (arg == "--selection-band" && hasValue(i, argc)) {
        selection_options.selection_band = std::stod(argv[++i]);
      } else if (arg == "--extra-margin" && hasValue(i, argc)) {
        selection_options.extra_margin = std::stod(argv[++i]);
      } else if (arg == "--phi-only") {
        query_options.compute_normals = false;
        query_options.output_mode = adasdf::SparseQueryOutputMode::PhiOnly;
      } else if (arg == "--with-normal") {
        query_options.compute_normals = true;
        query_options.output_mode = adasdf::SparseQueryOutputMode::PhiAndNormal;
      } else if (arg == "--early-exit") {
        query_options.early_exit = true;
      } else if (arg == "--no-radius") {
        selection_options.use_sample_radius = false;
        query_options.use_sample_radius = false;
      } else if (arg == "--no-neighbors") {
        selection_options.include_neighbor_blocks = false;
      } else if (arg == "--no-fallback") {
        query_options.fallback_to_model_query = false;
      } else if (arg == "--colliding-only") {
        query_options.include_non_colliding_samples = false;
      } else if (arg == "--sort") {
        query_options.sort_results_by_effective_phi = true;
      } else if (arg == "--cache-max-blocks" && hasValue(i, argc)) {
        cache_options.max_blocks =
            static_cast<std::size_t>(std::stoull(argv[++i]));
      } else if (arg == "--cache-max-mb" && hasValue(i, argc)) {
        cache_options.max_memory_bytes = static_cast<std::size_t>(
            std::stod(argv[++i]) * 1024.0 * 1024.0);
      } else if (arg == "--out" && hasValue(i, argc)) {
        out_path = argv[++i];
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

    const auto model = adasdf::SDFBinReader::read(model_path);
    if (!model || !model->isValid() || !model->queryBackendAvailable()) {
      std::cerr << "adasdf_active_block_query: failed to load queryable model: "
                << model_path.string() << "\n";
      return 1;
    }
    const auto samples =
        adasdf::CollisionSampleSetIO::readCSV(samples_path.string());
    if (!samples.success) {
      std::cerr << "adasdf_active_block_query: failed to read samples: "
                << samples.error_message << "\n";
      return 1;
    }

    adasdf::ExpandedBlockCache cache(cache_options);
    const adasdf::ActiveBlockQueryResult result =
        adasdf::ActiveBlockQuery::queryWithSelection(
            *model, samples.sample_set, cache, selection_options, query_options);
    if (!result.success) {
      std::cerr << "adasdf_active_block_query: query failed: "
                << result.error_message << "\n";
      return 2;
    }

    std::cout << "AdaSDF-CL active block query\n";
    std::cout << "Sample count: " << result.stats.sample_count << "\n";
    std::cout << "Queried count: " << result.stats.queried_count << "\n";
    std::cout << "Result count: " << result.stats.result_count << "\n";
    std::cout << "Cache queries: " << result.stats.cache_query_count << "\n";
    std::cout << "Fallback queries: " << result.stats.fallback_query_count << "\n";
    std::cout << "Resident blocks: " << result.stats.cache_stats.block_count << "\n";
    std::cout << "Resident memory bytes: "
              << result.stats.cache_stats.memory_bytes << "\n";
    std::cout << "Colliding: " << (result.colliding ? "true" : "false") << "\n";
    std::cout << "Min effective phi: " << result.stats.min_effective_phi << "\n";
    std::cout << "Query time ms: " << result.stats.query_time_ms << "\n";
    std::cout << "Status: ok\n";

    std::string error;
    if (!out_path.empty() &&
        !adasdf::ActiveBlockReportWriter::writeQueryCSV(
            out_path.string(), result, &error)) {
      std::cerr << "adasdf_active_block_query: failed to write CSV: "
                << error << "\n";
      return 2;
    }
    if (!report_path.empty() &&
        !writeText(
            report_path,
            adasdf::ActiveBlockReportWriter::queryToMarkdown(result))) {
      std::cerr << "adasdf_active_block_query: failed to write report\n";
      return 2;
    }
    if (!json_path.empty() &&
        !writeText(
            json_path,
            adasdf::ActiveBlockReportWriter::queryToJson(result))) {
      std::cerr << "adasdf_active_block_query: failed to write JSON report\n";
      return 2;
    }
    return result.colliding ? 10 : 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_active_block_query failed: " << exc.what() << "\n";
    return 2;
  }
}
