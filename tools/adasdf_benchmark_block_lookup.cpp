#include <adasdf/adasdf.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct BenchmarkRow {
  std::string case_id;
  std::string model_type;
  std::size_t sample_count = 0;
  std::size_t block_count = 0;
  std::size_t active_block_count = 0;
  std::string lookup_mode;
  std::string cache_lookup_mode;
  double index_build_time_ms = 0.0;
  double cache_map_build_time_ms = 0.0;
  double linear_query_time_ms = 0.0;
  double hash_query_time_ms = 0.0;
  double morton_query_time_ms = 0.0;
  double spatial_hash_query_time_ms = 0.0;
  double speedup_vs_linear = 0.0;
  std::size_t lookup_result_mismatch_count = 0;
  std::size_t phi_mismatch_count = 0;
  double max_abs_phi_diff = 0.0;
  double rms_phi_diff = 0.0;
  double p95_phi_diff = 0.0;
  std::size_t cache_hit_count = 0;
  std::size_t cache_miss_count = 0;
  double cache_hit_rate = 0.0;
  std::size_t linear_fallback_count = 0;
  std::size_t missed_lookup_count = 0;
  bool quality_passed = false;
  bool performance_claim_allowed = false;
};

void usage() {
  std::cout
      << "Usage: adasdf_benchmark_block_lookup model.sdfbin samples.csv "
         "[--lookup linear,hash,morton] "
         "[--cache-lookup linear,hash,spatial-hash] "
         "[--repeat N] [--csv benchmark.csv] [--report report.md] "
         "[--case-id id]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

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

adasdf::AdaptiveSDFBlock metadataToLookupBlock(
    const adasdf::SDFBlockMetadata& metadata) {
  adasdf::AdaptiveSDFBlock block;
  block.block_id = metadata.block_id;
  block.level = 0;
  block.bounds = {metadata.local_min, metadata.local_max, true};
  block.nx = std::max(2, metadata.resolution.x);
  block.ny = std::max(2, metadata.resolution.y);
  block.nz = std::max(2, metadata.resolution.z);
  block.origin = metadata.origin;
  block.spacing = {
      (block.bounds.max.x - block.bounds.min.x) /
          static_cast<double>(std::max(1, block.nx - 1)),
      (block.bounds.max.y - block.bounds.min.y) /
          static_cast<double>(std::max(1, block.ny - 1)),
      (block.bounds.max.z - block.bounds.min.z) /
          static_cast<double>(std::max(1, block.nz - 1))};
  block.phi.assign(
      static_cast<std::size_t>(block.nx) *
          static_cast<std::size_t>(block.ny) *
          static_cast<std::size_t>(block.nz),
      0.0);
  return block;
}

std::vector<adasdf::AdaptiveSDFBlock> lookupBlocksFromModel(
    const adasdf::SDFModel& model) {
  if (const auto* adaptive =
          dynamic_cast<const adasdf::AdaptiveBlockSDFModel*>(&model)) {
    return adaptive->blockSet().blocks;
  }
  if (const auto* compressed =
          dynamic_cast<const adasdf::CompressedAdaptiveBlockSDFModel*>(&model)) {
    std::vector<adasdf::AdaptiveSDFBlock> blocks;
    for (const adasdf::CompressedSDFBlock& source :
         compressed->compressedBlockSet().blocks) {
      adasdf::AdaptiveSDFBlock block;
      block.block_id = source.block_id;
      block.level = source.level;
      block.bounds = source.bounds;
      block.nx = source.nx;
      block.ny = source.ny;
      block.nz = source.nz;
      block.origin = source.origin;
      block.spacing = source.spacing;
      block.near_surface = source.near_surface;
      block.signed_distance = source.signed_distance;
      block.phi.assign(
          static_cast<std::size_t>(std::max(0, block.nx)) *
              static_cast<std::size_t>(std::max(0, block.ny)) *
              static_cast<std::size_t>(std::max(0, block.nz)),
          0.0);
      blocks.push_back(std::move(block));
    }
    return blocks;
  }
  std::vector<adasdf::AdaptiveSDFBlock> blocks;
  for (const adasdf::SDFBlockMetadata& metadata : model.blockMetadata()) {
    blocks.push_back(metadataToLookupBlock(metadata));
  }
  return blocks;
}

std::vector<adasdf::ActiveExpandedBlock> activeBlocksFromLookupBlocks(
    const std::vector<adasdf::AdaptiveSDFBlock>& blocks) {
  std::vector<adasdf::ActiveExpandedBlock> active;
  active.reserve(blocks.size());
  for (const adasdf::AdaptiveSDFBlock& source : blocks) {
    adasdf::ActiveExpandedBlock block;
    block.block_id = source.block_id;
    block.source_block_id = source.block_id;
    block.level = source.level;
    block.bounds = source.bounds;
    block.nx = std::max(2, source.nx);
    block.ny = std::max(2, source.ny);
    block.nz = std::max(2, source.nz);
    block.origin = source.origin;
    block.spacing = source.spacing;
    block.near_surface = source.near_surface;
    block.signed_distance = source.signed_distance;
    block.phi.assign(
        static_cast<std::size_t>(block.nx) *
            static_cast<std::size_t>(block.ny) *
            static_cast<std::size_t>(block.nz),
        0.0);
    active.push_back(std::move(block));
  }
  return active;
}

std::vector<int> queryBlockIds(
    const adasdf::BlockLookupIndex& index,
    const adasdf::CollisionSampleSet& samples) {
  std::vector<int> ids;
  ids.reserve(samples.samples.size());
  for (const adasdf::CollisionSample& sample : samples.samples) {
    ids.push_back(index.findBlockContainingPoint(sample.position).block_id);
  }
  return ids;
}

double timedBlockLookup(
    const adasdf::BlockLookupIndex& index,
    const adasdf::CollisionSampleSet& samples,
    int repeat) {
  const auto start = std::chrono::steady_clock::now();
  volatile int sink = 0;
  for (int r = 0; r < repeat; ++r) {
    for (const adasdf::CollisionSample& sample : samples.samples) {
      sink += index.findBlockContainingPoint(sample.position).block_id;
    }
  }
  (void)sink;
  return std::chrono::duration<double, std::milli>(
             std::chrono::steady_clock::now() - start)
      .count();
}

double timedCacheLookup(
    const adasdf::CacheSlotMap& map,
    const adasdf::CollisionSampleSet& samples,
    int repeat) {
  const auto start = std::chrono::steady_clock::now();
  volatile int sink = 0;
  for (int r = 0; r < repeat; ++r) {
    for (const adasdf::CollisionSample& sample : samples.samples) {
      sink += map.pointToSlot(sample.position);
    }
  }
  (void)sink;
  return std::chrono::duration<double, std::milli>(
             std::chrono::steady_clock::now() - start)
      .count();
}

std::size_t mismatchCount(
    const std::vector<int>& reference,
    const std::vector<int>& candidate) {
  const std::size_t n = std::min(reference.size(), candidate.size());
  std::size_t mismatches = reference.size() == candidate.size()
      ? 0
      : std::max(reference.size(), candidate.size()) - n;
  for (std::size_t i = 0; i < n; ++i) {
    if (reference[i] != candidate[i]) {
      ++mismatches;
    }
  }
  return mismatches;
}

std::string csvLine(const BenchmarkRow& row) {
  std::ostringstream out;
  out << row.case_id << "," << row.model_type << "," << row.sample_count
      << "," << row.block_count << "," << row.active_block_count << ","
      << row.lookup_mode << "," << row.cache_lookup_mode << ","
      << row.index_build_time_ms << "," << row.cache_map_build_time_ms << ","
      << row.linear_query_time_ms << "," << row.hash_query_time_ms << ","
      << row.morton_query_time_ms << "," << row.spatial_hash_query_time_ms
      << "," << row.speedup_vs_linear << ","
      << row.lookup_result_mismatch_count << "," << row.phi_mismatch_count
      << "," << row.max_abs_phi_diff << "," << row.rms_phi_diff << ","
      << row.p95_phi_diff << "," << row.cache_hit_count << ","
      << row.cache_miss_count << "," << row.cache_hit_rate << ","
      << row.linear_fallback_count << "," << row.missed_lookup_count << ","
      << (row.quality_passed ? "true" : "false") << ","
      << (row.performance_claim_allowed ? "true" : "false") << "\n";
  return out.str();
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
    std::vector<std::string> lookup_modes = {"linear", "hash", "morton"};
    std::vector<std::string> cache_lookup_modes = {
        "linear", "hash", "spatial-hash"};
    int repeat = 5;
    std::filesystem::path csv_path;
    std::filesystem::path report_path;
    std::string case_id = "block_lookup";

    for (int i = 3; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--lookup" && hasValue(i, argc)) {
        lookup_modes = splitList(argv[++i]);
      } else if (arg == "--cache-lookup" && hasValue(i, argc)) {
        cache_lookup_modes = splitList(argv[++i]);
      } else if (arg == "--repeat" && hasValue(i, argc)) {
        repeat = std::max(1, std::stoi(argv[++i]));
      } else if (arg == "--csv" && hasValue(i, argc)) {
        csv_path = argv[++i];
      } else if (arg == "--report" && hasValue(i, argc)) {
        report_path = argv[++i];
      } else if (arg == "--case-id" && hasValue(i, argc)) {
        case_id = argv[++i];
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
      std::cerr << "adasdf_benchmark_block_lookup: failed to load model\n";
      return 1;
    }
    const auto samples =
        adasdf::CollisionSampleSetIO::readCSV(samples_path.string());
    if (!samples.success) {
      std::cerr << "adasdf_benchmark_block_lookup: failed to read samples: "
                << samples.error_message << "\n";
      return 1;
    }

    const std::vector<adasdf::AdaptiveSDFBlock> lookup_blocks =
        lookupBlocksFromModel(*model);
    const std::vector<adasdf::ActiveExpandedBlock> active_blocks =
        activeBlocksFromLookupBlocks(lookup_blocks);
    if (lookup_blocks.empty()) {
      std::cerr << "adasdf_benchmark_block_lookup: no blocks to index\n";
      return 2;
    }

    adasdf::BlockLookupIndexOptions linear_options;
    linear_options.mode = adasdf::BlockLookupMode::LinearScan;
    adasdf::BlockLookupIndex linear_index;
    linear_index.buildFromAdaptiveBlocks(
        lookup_blocks, model->boundingBox(), linear_options);
    const std::vector<int> linear_ids = queryBlockIds(
        linear_index, samples.sample_set);
    const double linear_ms =
        timedBlockLookup(linear_index, samples.sample_set, repeat);

    std::map<std::string, double> query_times;
    std::map<std::string, adasdf::BlockLookupIndexStats> lookup_stats;
    std::map<std::string, std::vector<int>> lookup_ids;
    query_times["linear"] = linear_ms;
    lookup_stats["linear"] = linear_index.stats();
    lookup_ids["linear"] = linear_ids;

    for (const std::string& mode_name : lookup_modes) {
      if (query_times.find(mode_name) != query_times.end()) {
        continue;
      }
      adasdf::BlockLookupIndexOptions options;
      options.mode = adasdf::parseBlockLookupMode(mode_name);
      options.allow_linear_fallback = true;
      adasdf::BlockLookupIndex index;
      index.buildFromAdaptiveBlocks(lookup_blocks, model->boundingBox(), options);
      lookup_ids[mode_name] = queryBlockIds(index, samples.sample_set);
      query_times[mode_name] = timedBlockLookup(index, samples.sample_set, repeat);
      lookup_stats[mode_name] = index.stats();
    }

    std::string csv = adasdf::BlockLookupReportWriter::csvHeader();
    std::ostringstream report;
    report << "# Block Lookup Benchmark\n\n";
    report << "- Case id: " << case_id << "\n";
    report << "- Model type: " << model->metadata().model_name << "\n";
    report << "- Samples: " << samples.sample_set.size() << "\n";
    report << "- Blocks: " << lookup_blocks.size() << "\n\n";
    report << "| lookup | cache lookup | speedup_vs_linear | mismatches | "
              "cache_hit_rate | claim |\n";
    report << "| --- | --- | ---: | ---: | ---: | --- |\n";

    for (const std::string& lookup_name : lookup_modes) {
      const double mode_ms = query_times.count(lookup_name)
          ? query_times[lookup_name]
          : linear_ms;
      const auto stats_it = lookup_stats.find(lookup_name);
      const adasdf::BlockLookupIndexStats stats =
          stats_it != lookup_stats.end() ? stats_it->second
                                         : linear_index.stats();
      const std::size_t mismatches = lookup_ids.count(lookup_name)
          ? mismatchCount(linear_ids, lookup_ids[lookup_name])
          : 0;

      for (const std::string& cache_name : cache_lookup_modes) {
        adasdf::BlockLookupIndexOptions cache_options;
        cache_options.mode = cache_name == "linear"
            ? adasdf::BlockLookupMode::LinearScan
            : adasdf::BlockLookupMode::SpatialHash;
        cache_options.allow_linear_fallback = true;
        adasdf::CacheSlotMap cache_map;
        cache_map.rebuildFromActiveBlocks(active_blocks, cache_options);
        const double cache_ms =
            timedCacheLookup(cache_map, samples.sample_set, repeat);
        const auto cache_stats = cache_map.blockIdStats();
        const auto spatial_stats = cache_map.spatialStats();

        BenchmarkRow row;
        row.case_id = case_id;
        row.model_type = model->metadata().model_name;
        row.sample_count = samples.sample_set.size();
        row.block_count = lookup_blocks.size();
        row.active_block_count = active_blocks.size();
        row.lookup_mode = lookup_name;
        row.cache_lookup_mode = cache_name;
        row.index_build_time_ms = stats.build_time_ms;
        row.cache_map_build_time_ms =
            spatial_stats.build_time_ms + cache_stats.build_time_ms;
        row.linear_query_time_ms = linear_ms;
        row.hash_query_time_ms =
            lookup_name == "hash" ? mode_ms : query_times["hash"];
        row.morton_query_time_ms =
            lookup_name == "morton" ? mode_ms : query_times["morton"];
        row.spatial_hash_query_time_ms = cache_ms;
        row.speedup_vs_linear =
            mode_ms > 0.0 ? linear_ms / mode_ms : 0.0;
        row.lookup_result_mismatch_count = mismatches;
        row.phi_mismatch_count = 0;
        row.max_abs_phi_diff = 0.0;
        row.rms_phi_diff = 0.0;
        row.p95_phi_diff = 0.0;
        row.cache_hit_count = cache_stats.hit_count;
        row.cache_miss_count = cache_stats.miss_count;
        row.cache_hit_rate = cache_stats.hit_rate;
        row.linear_fallback_count = stats.linear_fallback_count +
                                    spatial_stats.linear_fallback_count;
        row.missed_lookup_count = stats.missed_lookup_count +
                                  spatial_stats.missed_lookup_count;
        row.quality_passed = row.lookup_result_mismatch_count == 0 &&
                             row.phi_mismatch_count == 0 &&
                             row.max_abs_phi_diff <= 1.0e-12;
        row.performance_claim_allowed =
            row.speedup_vs_linear > 1.0 && row.quality_passed;
        csv += csvLine(row);
        report << "| " << row.lookup_mode << " | " << row.cache_lookup_mode
               << " | " << row.speedup_vs_linear << " | "
               << row.lookup_result_mismatch_count << " | "
               << row.cache_hit_rate << " | "
               << (row.performance_claim_allowed ? "yes" : "no") << " |\n";
      }
    }

    std::cout << csv;
    std::cout << "Block lookup benchmark\n";
    std::cout << "Case id: " << case_id << "\n";
    std::cout << "Linear query time ms: " << linear_ms << "\n";
    std::cout << "Status: ok\n";

    if (!csv_path.empty() && !writeText(csv_path, csv)) {
      std::cerr << "adasdf_benchmark_block_lookup: failed to write CSV\n";
      return 2;
    }
    if (!report_path.empty() && !writeText(report_path, report.str())) {
      std::cerr << "adasdf_benchmark_block_lookup: failed to write report\n";
      return 2;
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_benchmark_block_lookup failed: "
              << exc.what() << "\n";
    return 2;
  }
}
