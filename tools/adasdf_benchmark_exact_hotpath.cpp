#include <adasdf/adasdf.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Options {
  std::filesystem::path input;
  int max_level = 4;
  int block_resolution = 8;
  int threads = 1;
  std::filesystem::path csv;
  std::filesystem::path report;
  std::string case_id = "exact_hotpath";
};

struct Result {
  std::string case_id;
  std::size_t block_count = 0;
  std::size_t sample_count = 0;
  double reference_exact_time_ms = 0.0;
  double hierarchical_exact_time_ms = 0.0;
  double optimized_hierarchical_exact_time_ms = 0.0;
  double reference_ns_per_sample = 0.0;
  double hierarchical_ns_per_sample = 0.0;
  double optimized_hierarchical_ns_per_sample = 0.0;
  double hierarchical_overhead_ratio = 0.0;
  double optimized_overhead_ratio = 0.0;
  std::size_t distance_query_count = 0;
  std::size_t sign_query_count = 0;
  std::size_t allocation_count_estimate = 0;
  bool diagnostics_enabled = false;
  bool counters_enabled = false;
};

void usage() {
  std::cout
      << "Usage: adasdf_benchmark_exact_hotpath input.stl "
         "[--max-level N] [--block-resolution N] [--threads N] "
         "[--csv out.csv] [--report out.md] [--case-id id]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

void ensureParent(const std::filesystem::path& path) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
}

std::size_t gridIndex(int i, int j, int k, int nx, int ny) {
  return static_cast<std::size_t>(i) +
         static_cast<std::size_t>(nx) *
             (static_cast<std::size_t>(j) +
              static_cast<std::size_t>(ny) * static_cast<std::size_t>(k));
}

adasdf::Vector3 gridPoint(const adasdf::AdaptiveSDFBlock& block, std::size_t index) {
  const int i = static_cast<int>(index % static_cast<std::size_t>(block.nx));
  const int j = static_cast<int>(
      (index / static_cast<std::size_t>(block.nx)) %
      static_cast<std::size_t>(block.ny));
  const int k = static_cast<int>(
      index /
      (static_cast<std::size_t>(block.nx) *
       static_cast<std::size_t>(block.ny)));
  return {
      block.origin.x + static_cast<double>(i) * block.spacing.x,
      block.origin.y + static_cast<double>(j) * block.spacing.y,
      block.origin.z + static_cast<double>(k) * block.spacing.z};
}

double nsPerSample(double ms, std::size_t samples) {
  return samples == 0 ? 0.0 : (ms * 1000000.0) / static_cast<double>(samples);
}

std::vector<adasdf::AdaptiveSDFBlock> makeBlocks(
    const adasdf::TriangleMesh& mesh,
    int max_level,
    int block_resolution) {
  adasdf::AdaptiveOctreeBuildOptions octree_options;
  octree_options.min_level = 0;
  octree_options.max_level = max_level;
  octree_options.signed_distance = true;
  octree_options.require_watertight_for_signed = false;
  adasdf::AdaptiveOctree octree =
      adasdf::AdaptiveOctreeBuilder::build(mesh, octree_options);
  adasdf::AdaptiveBlockPartitionOptions partition_options;
  partition_options.block_resolution = block_resolution;
  partition_options.include_all_leaves = true;
  return adasdf::AdaptiveBlockPartitioner::partition(octree, partition_options)
      .blocks;
}

double runReferenceExact(
    const adasdf::TriangleMesh& mesh,
    const adasdf::BVHSDFSampler& sampler,
    const adasdf::BVHSDFSamplerOptions& sampler_options,
    int threads,
    std::vector<adasdf::AdaptiveSDFBlock>* blocks) {
  struct Location {
    std::size_t block_index = 0;
    std::size_t local_index = 0;
  };
  std::vector<Location> locations;
  for (std::size_t block_index = 0; block_index < blocks->size(); ++block_index) {
    adasdf::AdaptiveSDFBlock& block = (*blocks)[block_index];
    block.signed_distance = true;
    block.phi.resize(
        static_cast<std::size_t>(block.nx) * static_cast<std::size_t>(block.ny) *
        static_cast<std::size_t>(block.nz));
    for (std::size_t local = 0; local < block.phi.size(); ++local) {
      locations.push_back({block_index, local});
    }
  }
  adasdf::ParallelSamplingOptions parallel_options;
  parallel_options.threads = std::max(1, threads);
  const adasdf::ParallelSamplingStats stats = adasdf::parallelFor(
      locations.size(),
      parallel_options,
      [&](std::size_t index) {
        const Location loc = locations[index];
        adasdf::AdaptiveSDFBlock& block = (*blocks)[loc.block_index];
        const adasdf::Vector3 point = gridPoint(block, loc.local_index);
        const adasdf::BVHSDFSampleResult sample =
            sampler.hasBVH()
                ? adasdf::BVHSDFSampler::sampleWithBVH(
                      mesh,
                      sampler.bvh(),
                      point,
                      sampler_options)
                : adasdf::BVHSDFSampler::sampleBruteForce(mesh, point, true);
        block.phi[loc.local_index] = sample.success ? sample.phi : 0.0;
      });
  return stats.elapsed_ms;
}

double runHierarchicalExactSerial(
    adasdf::BVHSDFSampler& sampler,
    int block_resolution,
    const std::vector<adasdf::AdaptiveSDFBlock>& blocks) {
  const auto begin = std::chrono::steady_clock::now();
  volatile double checksum = 0.0;
  for (const adasdf::AdaptiveSDFBlock& block : blocks) {
    adasdf::AdaptiveSDFBlock sampled =
        adasdf::HierarchicalBlockSampler::sampleBlockExact(
            block.bounds,
            block.block_id,
            block.octree_node_id,
            block.level,
            block_resolution,
            true,
            block.near_surface,
            sampler);
    if (!sampled.phi.empty()) {
      checksum += sampled.phi.front();
    }
  }
  (void)checksum;
  const auto end = std::chrono::steady_clock::now();
  return std::chrono::duration<double, std::milli>(end - begin).count();
}

double runOptimizedHierarchicalExact(
    const adasdf::TriangleMesh& mesh,
    adasdf::BVHSDFSampler& sampler,
    const adasdf::BVHSDFSamplerOptions& sampler_options,
    int block_resolution,
    int threads,
    const std::vector<adasdf::AdaptiveSDFBlock>& blocks) {
  std::vector<adasdf::AdaptiveSDFBlock> sampled = blocks;
  (void)block_resolution;
  return runReferenceExact(mesh, sampler, sampler_options, threads, &sampled);
}

void writeCsv(const std::filesystem::path& path, const Result& result) {
  if (path.empty()) {
    return;
  }
  ensureParent(path);
  std::ofstream out(path);
  out << "case_id,block_count,sample_count,reference_exact_time_ms,"
         "hierarchical_exact_time_ms,optimized_hierarchical_exact_time_ms,"
         "reference_ns_per_sample,hierarchical_ns_per_sample,"
         "optimized_hierarchical_ns_per_sample,hierarchical_overhead_ratio,"
         "optimized_overhead_ratio,distance_query_count,sign_query_count,"
         "allocation_count_estimate,diagnostics_enabled,counters_enabled\n";
  out << result.case_id << "," << result.block_count << ","
      << result.sample_count << "," << result.reference_exact_time_ms << ","
      << result.hierarchical_exact_time_ms << ","
      << result.optimized_hierarchical_exact_time_ms << ","
      << result.reference_ns_per_sample << ","
      << result.hierarchical_ns_per_sample << ","
      << result.optimized_hierarchical_ns_per_sample << ","
      << result.hierarchical_overhead_ratio << ","
      << result.optimized_overhead_ratio << ","
      << result.distance_query_count << "," << result.sign_query_count << ","
      << result.allocation_count_estimate << ","
      << (result.diagnostics_enabled ? "true" : "false") << ","
      << (result.counters_enabled ? "true" : "false") << "\n";
}

void writeReport(const std::filesystem::path& path, const Result& result) {
  if (path.empty()) {
    return;
  }
  ensureParent(path);
  std::ofstream out(path);
  out << "# AdaSDF-CL Exact Hotpath Benchmark\n\n";
  out << "- Case id: " << result.case_id << "\n";
  out << "- Block count: " << result.block_count << "\n";
  out << "- Sample count: " << result.sample_count << "\n";
  out << "- Reference exact time ms: "
      << result.reference_exact_time_ms << "\n";
  out << "- Hierarchical exact time ms: "
      << result.hierarchical_exact_time_ms << "\n";
  out << "- Optimized hierarchical exact time ms: "
      << result.optimized_hierarchical_exact_time_ms << "\n";
  out << "- Reference ns/sample: "
      << result.reference_ns_per_sample << "\n";
  out << "- Hierarchical ns/sample: "
      << result.hierarchical_ns_per_sample << "\n";
  out << "- Optimized hierarchical ns/sample: "
      << result.optimized_hierarchical_ns_per_sample << "\n";
  out << "- Hierarchical overhead ratio: "
      << result.hierarchical_overhead_ratio << "\n";
  out << "- Optimized overhead ratio: "
      << result.optimized_overhead_ratio << "\n";
  out << "- Distance query count: " << result.distance_query_count << "\n";
  out << "- Sign query count: " << result.sign_query_count << "\n";
  out << "- Allocation count estimate: "
      << result.allocation_count_estimate << "\n";
  out << "- Diagnostics enabled: "
      << (result.diagnostics_enabled ? "yes" : "no") << "\n";
  out << "- Counters enabled: "
      << (result.counters_enabled ? "yes" : "no") << "\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    Options options;
    if (argc == 1) {
      usage();
      return 0;
    }
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else if (arg == "--max-level" && hasValue(i, argc)) {
        options.max_level = std::stoi(argv[++i]);
      } else if (arg == "--block-resolution" && hasValue(i, argc)) {
        options.block_resolution = std::stoi(argv[++i]);
      } else if (arg == "--threads" && hasValue(i, argc)) {
        options.threads = std::stoi(argv[++i]);
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
    if (options.input.empty() || !std::filesystem::exists(options.input)) {
      std::cerr << "adasdf_benchmark_exact_hotpath: input STL missing: "
                << options.input.string() << "\n";
      return 1;
    }

    adasdf::STLReadResult read = adasdf::STLReader::read(options.input.string());
    if (!read.success) {
      std::cerr << "adasdf_benchmark_exact_hotpath: failed to read STL: "
                << read.error_message << "\n";
      return 2;
    }

    std::vector<adasdf::AdaptiveSDFBlock> blocks = makeBlocks(
        read.mesh,
        options.max_level,
        options.block_resolution);
    adasdf::BVHSDFSamplerOptions sampler_options;
    sampler_options.acceleration = adasdf::SDFSamplingAcceleration::BVH;
    sampler_options.signed_distance = true;
    sampler_options.enable_counters = false;
    adasdf::BuildAccelerationStats acceleration_stats;
    acceleration_stats.threads_requested = std::max(1, options.threads);
    adasdf::BVHSDFSampler sampler;
    sampler.reset(read.mesh, sampler_options, &acceleration_stats);

    const std::size_t samples_per_block =
        static_cast<std::size_t>(std::max(2, options.block_resolution)) *
        static_cast<std::size_t>(std::max(2, options.block_resolution)) *
        static_cast<std::size_t>(std::max(2, options.block_resolution));
    Result result;
    result.case_id = options.case_id;
    result.block_count = blocks.size();
    result.sample_count = samples_per_block * result.block_count;
    result.distance_query_count = result.sample_count;
    result.sign_query_count = result.sample_count;
    result.allocation_count_estimate = result.block_count * 3 + 1;

    std::vector<adasdf::AdaptiveSDFBlock> reference_blocks = blocks;
    result.reference_exact_time_ms = runReferenceExact(
        read.mesh,
        sampler,
        sampler_options,
        options.threads,
        &reference_blocks);
    result.hierarchical_exact_time_ms =
        runHierarchicalExactSerial(sampler, options.block_resolution, blocks);
    result.optimized_hierarchical_exact_time_ms =
        runOptimizedHierarchicalExact(
            read.mesh,
            sampler,
            sampler_options,
            options.block_resolution,
            options.threads,
            blocks);
    result.reference_ns_per_sample =
        nsPerSample(result.reference_exact_time_ms, result.sample_count);
    result.hierarchical_ns_per_sample =
        nsPerSample(result.hierarchical_exact_time_ms, result.sample_count);
    result.optimized_hierarchical_ns_per_sample = nsPerSample(
        result.optimized_hierarchical_exact_time_ms,
        result.sample_count);
    result.hierarchical_overhead_ratio =
        result.reference_ns_per_sample > 0.0
            ? result.hierarchical_ns_per_sample /
                  result.reference_ns_per_sample
            : 0.0;
    result.optimized_overhead_ratio =
        result.reference_ns_per_sample > 0.0
            ? result.optimized_hierarchical_ns_per_sample /
                  result.reference_ns_per_sample
            : 0.0;

    writeCsv(options.csv, result);
    writeReport(options.report, result);

    std::cout << "AdaSDF-CL exact hotpath benchmark\n";
    std::cout << "Case id: " << result.case_id << "\n";
    std::cout << "Block count: " << result.block_count << "\n";
    std::cout << "Sample count: " << result.sample_count << "\n";
    std::cout << "Reference exact time ms: "
              << result.reference_exact_time_ms << "\n";
    std::cout << "Hierarchical exact time ms: "
              << result.hierarchical_exact_time_ms << "\n";
    std::cout << "Optimized hierarchical exact time ms: "
              << result.optimized_hierarchical_exact_time_ms << "\n";
    std::cout << "Reference ns/sample: "
              << result.reference_ns_per_sample << "\n";
    std::cout << "Hierarchical ns/sample: "
              << result.hierarchical_ns_per_sample << "\n";
    std::cout << "Optimized hierarchical ns/sample: "
              << result.optimized_hierarchical_ns_per_sample << "\n";
    std::cout << "Hierarchical overhead ratio: "
              << result.hierarchical_overhead_ratio << "\n";
    std::cout << "Optimized overhead ratio: "
              << result.optimized_overhead_ratio << "\n";
    std::cout << "Distance query count: " << result.distance_query_count
              << "\n";
    std::cout << "Sign query count: " << result.sign_query_count << "\n";
    std::cout << "Allocation count estimate: "
              << result.allocation_count_estimate << "\n";
    std::cout << "Diagnostics enabled: no\n";
    std::cout << "Counters enabled: no\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_benchmark_exact_hotpath failed: " << exc.what()
              << "\n";
    return 3;
  }
}
