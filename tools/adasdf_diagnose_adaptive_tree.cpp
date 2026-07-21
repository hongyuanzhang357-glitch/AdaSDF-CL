#include <adasdf/adasdf.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

enum class DiagnoseSamplingMode {
  Exact,
  ContactBand,
  Hierarchical,
};

struct Options {
  std::filesystem::path input;
  std::filesystem::path json_path;
  std::filesystem::path report_path;
  std::filesystem::path profile_json_path;

  int min_level = 1;
  int max_level = 3;
  int block_resolution = 8;
  int threads = 1;

  double padding = 0.05;
  double target_error = 1e-3;
  double surface_band_factor = 1.5;
  double max_seconds = 0.0;

  bool signed_distance = true;
  bool require_watertight = true;

  adasdf::AdaptiveLeafMode leaf_mode = adasdf::AdaptiveLeafMode::Mixed;
  adasdf::SDFSamplingAcceleration acceleration =
      adasdf::SDFSamplingAcceleration::BVH;
  DiagnoseSamplingMode sampling = DiagnoseSamplingMode::Exact;

  adasdf::ContactBandSamplingOptions contact_band;
  adasdf::HierarchicalSamplingOptions hierarchical;
};

void usage() {
  std::cout
      << "Usage: adasdf_diagnose_adaptive_tree input.stl "
         "[--json stats.json] [--report stats.md] "
         "[--min-level N] [--max-level N] [--block-resolution N] "
         "[--adaptive-leaf-mode mixed|uniform] "
         "[--sampling exact|contact-band|hierarchical] "
         "[--target-error value] [--surface-band-factor value] "
         "[--padding value] [--signed|--unsigned] [--allow-open-unsigned] "
         "[--accel brute|bvh] [--threads N] [--max-seconds value] "
         "[--contact-band-width value] [--contact-band-layers N] "
         "[--halo-exact-layers N] [--far-field-resolution N] "
         "[--contact-band-marker conservative-aabb|distance-aware|hybrid] "
         "[--marker-cell-size-factor value] [--marker-safety-factor value] "
         "[--marker-min-band value] [--marker-max-band value] "
         "[--local-halo-only] [--profile-json profile.json]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

bool parseSamplingMode(const std::string& text, DiagnoseSamplingMode* out) {
  if (out == nullptr) {
    return false;
  }
  if (text == "exact") {
    *out = DiagnoseSamplingMode::Exact;
    return true;
  }
  if (text == "contact-band") {
    *out = DiagnoseSamplingMode::ContactBand;
    return true;
  }
  if (text == "hierarchical") {
    *out = DiagnoseSamplingMode::Hierarchical;
    return true;
  }
  return false;
}

const char* toString(DiagnoseSamplingMode mode) {
  switch (mode) {
    case DiagnoseSamplingMode::Exact:
      return "exact";
    case DiagnoseSamplingMode::ContactBand:
      return "contact-band";
    case DiagnoseSamplingMode::Hierarchical:
      return "hierarchical";
  }
  return "exact";
}

int parseThreads(const std::string& value) {
  if (value == "auto") {
    const unsigned int hardware = std::thread::hardware_concurrency();
    return static_cast<int>(hardware == 0 ? 1 : hardware);
  }
  return std::max(1, std::stoi(value));
}

bool expired(
    const std::chrono::steady_clock::time_point& start,
    double max_seconds) {
  if (max_seconds <= 0.0) {
    return false;
  }
  const double elapsed =
      std::chrono::duration<double>(
          std::chrono::steady_clock::now() - start)
          .count();
  return elapsed >= max_seconds;
}

adasdf::AdaptiveTreeBlockSamplingStats exactBlockStats(
    const adasdf::AdaptiveSDFBlock& block) {
  adasdf::AdaptiveTreeBlockSamplingStats stats;
  stats.block_id = block.block_id;
  stats.level = block.level;
  stats.logical_node_count =
      static_cast<std::size_t>(block.nx) *
      static_cast<std::size_t>(block.ny) *
      static_cast<std::size_t>(block.nz);
  stats.exact_node_count = stats.logical_node_count;
  return stats;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1) {
      usage();
      return 0;
    }

    Options options;
    options.contact_band.enable_contact_band_sampling = true;
    options.hierarchical.enable_hierarchical_sampling = true;

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else if (arg == "--json" && hasValue(i, argc)) {
        options.json_path = argv[++i];
      } else if (arg == "--report" && hasValue(i, argc)) {
        options.report_path = argv[++i];
      } else if (arg == "--profile-json" && hasValue(i, argc)) {
        options.profile_json_path = argv[++i];
      } else if (arg == "--min-level" && hasValue(i, argc)) {
        options.min_level = std::stoi(argv[++i]);
      } else if (arg == "--max-level" && hasValue(i, argc)) {
        options.max_level = std::stoi(argv[++i]);
      } else if (arg == "--block-resolution" && hasValue(i, argc)) {
        options.block_resolution = std::stoi(argv[++i]);
      } else if (arg == "--adaptive-leaf-mode" && hasValue(i, argc)) {
        if (!adasdf::parseAdaptiveLeafMode(argv[++i], &options.leaf_mode)) {
          std::cerr << "Unknown adaptive leaf mode: " << argv[i] << "\n";
          return 1;
        }
      } else if (arg == "--sampling" && hasValue(i, argc)) {
        if (!parseSamplingMode(argv[++i], &options.sampling)) {
          std::cerr << "Unknown sampling mode: " << argv[i] << "\n";
          return 1;
        }
      } else if (arg == "--target-error" && hasValue(i, argc)) {
        options.target_error = std::stod(argv[++i]);
      } else if (arg == "--surface-band-factor" && hasValue(i, argc)) {
        options.surface_band_factor = std::stod(argv[++i]);
      } else if (arg == "--padding" && hasValue(i, argc)) {
        options.padding = std::stod(argv[++i]);
      } else if (arg == "--signed") {
        options.signed_distance = true;
        options.require_watertight = true;
      } else if (arg == "--unsigned") {
        options.signed_distance = false;
        options.require_watertight = false;
      } else if (arg == "--allow-open-unsigned") {
        options.signed_distance = false;
        options.require_watertight = false;
      } else if ((arg == "--accel" || arg == "--distance-backend") &&
                 hasValue(i, argc)) {
        std::string value = argv[++i];
        if (value == "brute_force") {
          value = "brute";
        }
        if (!adasdf::parseSDFSamplingAcceleration(value, &options.acceleration)) {
          std::cerr << "Unknown acceleration mode: " << argv[i] << "\n";
          return 1;
        }
      } else if (arg == "--threads" && hasValue(i, argc)) {
        options.threads = parseThreads(argv[++i]);
      } else if (arg == "--max-seconds" && hasValue(i, argc)) {
        options.max_seconds = std::stod(argv[++i]);
      } else if (arg == "--contact-band-width" && hasValue(i, argc)) {
        options.contact_band.contact_band_width = std::stod(argv[++i]);
      } else if (arg == "--contact-band-layers" && hasValue(i, argc)) {
        options.contact_band.contact_band_layers = std::stoi(argv[++i]);
      } else if (arg == "--halo-exact-layers" && hasValue(i, argc)) {
        options.contact_band.halo_exact_layers = std::stoi(argv[++i]);
        options.hierarchical.halo_exact_layers =
            options.contact_band.halo_exact_layers;
      } else if (arg == "--far-field-resolution" && hasValue(i, argc)) {
        options.contact_band.far_field_resolution = std::stoi(argv[++i]);
      } else if (arg == "--far-field-mode" && hasValue(i, argc)) {
        adasdf::ContactBandFarFieldMode mode;
        if (!adasdf::parseContactBandFarFieldMode(argv[++i], &mode)) {
          std::cerr << "Unknown contact-band far-field mode: " << argv[i]
                    << "\n";
          return 1;
        }
        options.contact_band.far_field_mode = mode;
      } else if (arg == "--contact-band-marker" && hasValue(i, argc)) {
        adasdf::ContactBandMarkerMode mode;
        if (!adasdf::parseContactBandMarkerMode(argv[++i], &mode)) {
          std::cerr << "Unknown contact-band marker: " << argv[i] << "\n";
          return 1;
        }
        options.contact_band.marker_mode = mode;
      } else if (arg == "--marker-cell-size-factor" && hasValue(i, argc)) {
        options.contact_band.marker_cell_size_factor = std::stod(argv[++i]);
      } else if (arg == "--marker-safety-factor" && hasValue(i, argc)) {
        options.contact_band.marker_safety_factor = std::stod(argv[++i]);
      } else if (arg == "--marker-min-band" && hasValue(i, argc)) {
        options.contact_band.marker_min_band = std::stod(argv[++i]);
      } else if (arg == "--marker-max-band" && hasValue(i, argc)) {
        options.contact_band.marker_max_band = std::stod(argv[++i]);
      } else if (arg == "--local-halo-only") {
        options.contact_band.local_halo_only = true;
        options.contact_band.disable_global_halo = true;
      } else if (arg == "--disable-global-halo") {
        options.contact_band.disable_global_halo = true;
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
      std::cerr << "adasdf_diagnose_adaptive_tree: input STL does not exist: "
                << options.input.string() << "\n";
      return 1;
    }

    const auto start = std::chrono::steady_clock::now();
    const adasdf::STLReadResult read =
        adasdf::STLReader::read(options.input.string());
    if (!read.success) {
      std::cerr << "adasdf_diagnose_adaptive_tree: failed to read STL: "
                << read.error_message << "\n";
      return 1;
    }

    adasdf::AdaptiveOctreeBuildOptions octree_options;
    octree_options.min_level = options.min_level;
    octree_options.max_level = options.max_level;
    octree_options.leaf_mode = options.leaf_mode;
    octree_options.padding = options.padding;
    octree_options.target_near_surface_error = options.target_error;
    octree_options.surface_band_factor = options.surface_band_factor;
    octree_options.signed_distance = options.signed_distance;
    octree_options.require_watertight_for_signed = options.require_watertight;
    octree_options.distance_backend = options.acceleration;

    adasdf::AdaptiveOctreeBuildReport octree_report;
    adasdf::AdaptiveOctree octree =
        adasdf::AdaptiveOctreeBuilder::build(
            read.mesh,
            octree_options,
            &octree_report);
    if (!octree_report.success) {
      std::cerr << "adasdf_diagnose_adaptive_tree: octree build failed: "
                << octree_report.error_message << "\n";
      return 2;
    }

    adasdf::AdaptiveBlockPartitionOptions partition_options;
    partition_options.block_resolution = options.block_resolution;
    partition_options.include_all_leaves = true;
    adasdf::AdaptiveSDFBlockSet blocks =
        adasdf::AdaptiveBlockPartitioner::partition(octree, partition_options);
    blocks.signed_distance = options.signed_distance;

    std::vector<adasdf::AdaptiveTreeBlockSamplingStats> block_stats;
    block_stats.reserve(blocks.blocks.size());
    bool timed_out = false;
    std::string timeout_stage;

    if (options.sampling == DiagnoseSamplingMode::Exact) {
      for (const adasdf::AdaptiveSDFBlock& block : blocks.blocks) {
        block_stats.push_back(exactBlockStats(block));
      }
    } else {
      adasdf::BuildAccelerationStats acceleration_stats;
      adasdf::BVHSDFSamplerOptions sampler_options;
      sampler_options.acceleration = options.acceleration;
      sampler_options.signed_distance = options.signed_distance;
      adasdf::BVHSDFSampler sampler;
      if (!sampler.reset(read.mesh, sampler_options, &acceleration_stats)) {
        std::cerr << "adasdf_diagnose_adaptive_tree: failed to build sampler\n";
        return 3;
      }

      for (std::size_t index = 0; index < blocks.blocks.size(); ++index) {
        if (expired(start, options.max_seconds)) {
          timed_out = true;
          timeout_stage = "adaptive_tree_sampling";
          break;
        }
        const adasdf::AdaptiveSDFBlock& block = blocks.blocks[index];
        if (options.sampling == DiagnoseSamplingMode::ContactBand) {
          adasdf::ContactBandMarkerCache marker_cache;
          const auto sampled = adasdf::ContactBandBlockSampler::sampleBlock(
              block.bounds,
              block.block_id,
              block.octree_node_id,
              block.level,
              options.block_resolution,
              options.signed_distance,
              sampler,
              sampler.bvh(),
              options.contact_band,
              nullptr,
              &marker_cache);
          if (!sampled.success) {
            std::cerr << "adasdf_diagnose_adaptive_tree: block "
                      << block.block_id << " contact-band sampling failed: "
                      << sampled.error_message << "\n";
            return 4;
          }
          adasdf::AdaptiveTreeBlockSamplingStats stats;
          stats.block_id = block.block_id;
          stats.level = block.level;
          stats.contact_band = sampled.has_contact_band;
          stats.far_field = !sampled.has_contact_band;
          stats.exact_node_count = sampled.exact_node_count;
          stats.predicted_node_count = sampled.predicted_node_count;
          stats.far_field_node_count = sampled.far_field_node_count;
          stats.logical_node_count = sampled.block.phi.size();
          block_stats.push_back(stats);
        } else {
          adasdf::HierarchicalSamplingOptions hierarchical =
              options.hierarchical;
          hierarchical.threads = options.threads;
          hierarchical.target_max_abs_error = options.target_error;
          hierarchical.target_rms_error = options.target_error;
          hierarchical.target_p95_error = options.target_error;
          const auto sampled = adasdf::HierarchicalBlockSampler::sampleBlock(
              block.bounds,
              block.block_id,
              block.octree_node_id,
              block.level,
              options.block_resolution,
              options.signed_distance,
              block.near_surface,
              sampler,
              hierarchical);
          if (!sampled.success) {
            std::cerr << "adasdf_diagnose_adaptive_tree: block "
                      << block.block_id << " hierarchical sampling failed: "
                      << sampled.error_message << "\n";
            return 4;
          }
          adasdf::AdaptiveTreeBlockSamplingStats stats;
          stats.block_id = block.block_id;
          stats.level = block.level;
          stats.contact_band = block.near_surface;
          stats.far_field = !block.near_surface;
          stats.exact_node_count = sampled.exact_sample_count;
          stats.predicted_node_count = sampled.predicted_sample_count;
          stats.far_field_node_count =
              sampled.decision.importance ==
                      adasdf::BlockImportanceClass::FarField
                  ? sampled.block.phi.size()
                  : 0;
          stats.logical_node_count = sampled.block.phi.size();
          block_stats.push_back(stats);
        }
      }
    }

    adasdf::AdaptiveTreeStats stats = adasdf::computeAdaptiveTreeStats(
        octree,
        blocks,
        options.block_resolution,
        options.max_level,
        block_stats);

    if (!adasdf::AdaptiveTreeStatsWriter::writeJson(
            options.json_path.string(),
            stats,
            "adasdf_diagnose_adaptive_tree")) {
      std::cerr << "adasdf_diagnose_adaptive_tree: failed to write JSON\n";
      return 5;
    }
    if (!adasdf::AdaptiveTreeStatsWriter::writeMarkdown(
            options.report_path.string(),
            stats)) {
      std::cerr << "adasdf_diagnose_adaptive_tree: failed to write report\n";
      return 5;
    }
    if (!adasdf::AdaptiveTreeStatsWriter::writeJson(
            options.profile_json_path.string(),
            stats,
            "adasdf_diagnose_adaptive_tree_profile")) {
      std::cerr << "adasdf_diagnose_adaptive_tree: failed to write profile JSON\n";
      return 5;
    }

    const double elapsed_ms =
        std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start)
            .count();
    std::cout << "AdaSDF-CL adaptive tree diagnostics\n";
    std::cout << "Input: " << options.input.string() << "\n";
    std::cout << "Adaptive leaf mode: "
              << adasdf::toString(options.leaf_mode) << "\n";
    std::cout << "Sampling: " << toString(options.sampling) << "\n";
    std::cout << "Leaf blocks: " << stats.leaf_block_count << "\n";
    std::cout << "Octree nodes: " << stats.octree_node_count << "\n";
    std::cout << "Max level used: " << stats.max_level_used << "\n";
    std::cout << "Total logical nodes: "
              << stats.total_logical_node_count << "\n";
    std::cout << "Uniform max-level logical nodes: "
              << stats.uniform_max_level_logical_node_count << "\n";
    std::cout << "Sparsity ratio vs uniform max-level: "
              << stats.sparsity_ratio_vs_uniform_max_level << "\n";
    std::cout << "Appears uniform max-level: "
              << (stats.appears_uniform_max_level ? "yes" : "no") << "\n";
    std::cout << "Mixed level present: "
              << (stats.mixed_level_present ? "yes" : "no") << "\n";
    std::cout << "Elapsed ms: " << elapsed_ms << "\n";
    if (!options.json_path.empty()) {
      std::cout << "JSON: " << options.json_path.string() << "\n";
    }
    if (!options.report_path.empty()) {
      std::cout << "Report: " << options.report_path.string() << "\n";
    }
    if (timed_out) {
      std::cerr << "adasdf_diagnose_adaptive_tree: timed out in "
                << timeout_stage << "\n";
      return adasdf::toInt(adasdf::CliExitCode::Timeout);
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_diagnose_adaptive_tree failed: " << exc.what()
              << "\n";
    return 3;
  }
}
