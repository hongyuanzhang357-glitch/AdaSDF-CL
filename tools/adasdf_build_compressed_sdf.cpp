#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_build_compressed_sdf input.stl output_compressed.sdfbin "
         "[--target-error 1e-3] [--max-level N] [--min-level N] "
         "[--block-resolution N] [--padding 0.05] [--signed|--unsigned] "
         "[--auto-clean] [--fixed-rank N] [--min-rank N] [--max-rank N] "
         "[--keep-near-surface-dense] [--report build_report.md] "
         "[--accel brute|bvh] [--threads N] [--benchmark-brute-reference] "
         "[--sampling exact|hierarchical|contact-band] [--coarse-resolution N] "
         "[--quality-check-samples N] [--far-field-interpolation] "
         "[--no-far-field-interpolation] [--transition-prediction] "
         "[--no-transition-prediction] [--near-surface-exact] "
         "[--quality-guard] [--no-quality-guard] "
         "[--contact-band-width value] [--contact-band-layers N] "
         "[--halo-exact-layers N] [--far-field-resolution N] "
         "[--far-field-mode coarse-interpolate|constant-sign|clamped-distance] "
         "[--reuse-far-field-sign] [--no-reuse-far-field-sign] "
         "[--audit contact-band|global] [--contact-band-normal-audit] "
         "[--contact-band-normal-error-limit-deg value] "
         "[--target-sampling-error 1e-3] "
         "[--compression-report compression_report.md] "
         "[--quality-report quality_report.md] "
         "[--strict-json report.json] [--case-id case_id] "
         "[--recommend] [--verbose]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

bool parseAcceleration(
    const std::string& value,
    adasdf::AdaptiveBlockSDFBuildOptions* options) {
  adasdf::SDFSamplingAcceleration acceleration;
  if (!adasdf::parseSDFSamplingAcceleration(value, &acceleration)) {
    return false;
  }
  options->acceleration = acceleration;
  return true;
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

void setTargetSamplingError(
    adasdf::AdaptiveBlockSDFBuildOptions* options,
    double value) {
  options->hierarchical_sampling.target_max_abs_error = value;
  options->hierarchical_sampling.target_rms_error = value;
  options->hierarchical_sampling.target_p95_error = value;
}

const char* samplingModeName(const adasdf::AdaptiveBlockSDFBuildOptions& options) {
  if (options.contact_band_sampling.enable_contact_band_sampling) {
    return "contact-band";
  }
  return options.hierarchical_sampling.enable_hierarchical_sampling
             ? "hierarchical"
             : "exact";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1) {
      usage();
      return 0;
    }

    std::filesystem::path input;
    std::filesystem::path output;
    std::filesystem::path build_report_path;
    std::filesystem::path compression_report_path;
    std::filesystem::path quality_report_path;
    std::filesystem::path strict_json_path;
    std::string case_id = "default";
    adasdf::AdaptiveBlockSDFBuildOptions build_options;
    adasdf::BlockLowRankCompressionOptions compression_options;
    const auto strict_timer = adasdf::startStrictRunTimer();
    std::map<std::string, std::string> strict_parameters =
        adasdf::commandLineParameters(argc, argv);
    auto write_strict =
        [&](bool success,
            const std::string& status,
            const std::string& failure_reason,
            const std::map<std::string, double>& metrics = {}) {
          if (strict_json_path.empty()) {
            return;
          }
          std::string strict_error;
          if (!adasdf::writeStrictRunReport(
                  strict_json_path,
                  "adasdf_build_compressed_sdf",
                  case_id,
                  input,
                  output,
                  strict_parameters,
                  metrics,
                  success,
                  status,
                  failure_reason,
                  strict_timer,
                  &strict_error)) {
            std::cerr << "adasdf_build_compressed_sdf: failed to write "
                         "strict JSON: "
                      << strict_error << "\n";
          }
        };

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else if (arg == "--recommend") {
        std::cout
            << "Use adasdf_recommend_build for parameter recommendation.\n";
        return 0;
      } else if (arg == "--target-error" && hasValue(i, argc)) {
        const double value = std::stod(argv[++i]);
        build_options.target_near_surface_error = value;
        compression_options.target_max_abs_error = value;
        compression_options.target_p95_error = value;
      } else if (arg == "--max-level" && hasValue(i, argc)) {
        build_options.max_octree_level = std::stoi(argv[++i]);
      } else if (arg == "--min-level" && hasValue(i, argc)) {
        build_options.min_octree_level = std::stoi(argv[++i]);
      } else if (arg == "--block-resolution" && hasValue(i, argc)) {
        build_options.block_resolution = std::stoi(argv[++i]);
      } else if (arg == "--padding" && hasValue(i, argc)) {
        build_options.padding = std::stod(argv[++i]);
      } else if (arg == "--signed") {
        build_options.signed_distance = true;
      } else if (arg == "--unsigned") {
        build_options.signed_distance = false;
        build_options.require_watertight_for_signed = false;
      } else if (arg == "--auto-clean") {
        build_options.auto_safe_cleanup = true;
      } else if (arg == "--accel" && hasValue(i, argc)) {
        if (!parseAcceleration(argv[++i], &build_options)) {
          std::cerr << "Unknown acceleration mode: " << argv[i] << "\n";
          return 1;
        }
      } else if (arg == "--threads" && hasValue(i, argc)) {
        build_options.threads = std::stoi(argv[++i]);
      } else if (arg == "--benchmark-brute-reference") {
        build_options.benchmark_brute_reference = true;
      } else if (arg == "--sampling" && hasValue(i, argc)) {
        if (!parseSamplingMode(argv[++i], &build_options)) {
          std::cerr << "Unknown sampling mode: " << argv[i] << "\n";
          return 1;
        }
      } else if (arg == "--coarse-resolution" && hasValue(i, argc)) {
        build_options.hierarchical_sampling.coarse_resolution =
            std::stoi(argv[++i]);
      } else if (arg == "--quality-check-samples" && hasValue(i, argc)) {
        build_options.hierarchical_sampling.quality_check_samples_per_axis =
            std::stoi(argv[++i]);
      } else if (arg == "--far-field-interpolation") {
        build_options.hierarchical_sampling.allow_far_field_interpolation =
            true;
      } else if (arg == "--no-far-field-interpolation") {
        build_options.hierarchical_sampling.allow_far_field_interpolation =
            false;
      } else if (arg == "--transition-prediction") {
        build_options.hierarchical_sampling.allow_transition_prediction = true;
      } else if (arg == "--no-transition-prediction") {
        build_options.hierarchical_sampling.allow_transition_prediction = false;
      } else if (arg == "--near-surface-exact") {
        build_options.hierarchical_sampling.keep_near_surface_exact = true;
      } else if (arg == "--quality-guard") {
        build_options.hierarchical_sampling.quality_guard = true;
      } else if (arg == "--no-quality-guard") {
        build_options.hierarchical_sampling.quality_guard = false;
      } else if (arg == "--contact-band-width" && hasValue(i, argc)) {
        build_options.contact_band_sampling.contact_band_width =
            std::stod(argv[++i]);
      } else if (arg == "--contact-band-layers" && hasValue(i, argc)) {
        build_options.contact_band_sampling.contact_band_layers =
            std::stoi(argv[++i]);
      } else if (arg == "--halo-exact-layers" && hasValue(i, argc)) {
        build_options.contact_band_sampling.halo_exact_layers =
            std::stoi(argv[++i]);
      } else if (arg == "--far-field-resolution" && hasValue(i, argc)) {
        build_options.contact_band_sampling.far_field_resolution =
            std::stoi(argv[++i]);
      } else if (arg == "--far-field-mode" && hasValue(i, argc)) {
        adasdf::ContactBandFarFieldMode mode;
        if (!adasdf::parseContactBandFarFieldMode(argv[++i], &mode)) {
          std::cerr << "Unknown contact-band far-field mode: " << argv[i]
                    << "\n";
          return 1;
        }
        build_options.contact_band_sampling.far_field_mode = mode;
      } else if (arg == "--reuse-far-field-sign") {
        build_options.contact_band_sampling.reuse_far_field_sign = true;
      } else if (arg == "--no-reuse-far-field-sign") {
        build_options.contact_band_sampling.reuse_far_field_sign = false;
      } else if (arg == "--audit" && hasValue(i, argc)) {
        const std::string audit = argv[++i];
        if (audit == "contact-band") {
          build_options.contact_band_sampling.audit_contact_band_only = true;
          build_options.contact_band_sampling.global_quality_gate = false;
        } else if (audit == "global") {
          build_options.contact_band_sampling.audit_contact_band_only = false;
          build_options.contact_band_sampling.global_quality_gate = true;
        } else {
          std::cerr << "Unknown audit mode: " << audit << "\n";
          return 1;
        }
      } else if (arg == "--contact-band-normal-audit") {
        build_options.contact_band_sampling.normal_audit = true;
      } else if (arg == "--contact-band-normal-error-limit-deg" &&
                 hasValue(i, argc)) {
        build_options.contact_band_sampling.contact_band_normal_error_limit_deg =
            std::stod(argv[++i]);
      } else if (arg == "--target-sampling-error" && hasValue(i, argc)) {
        setTargetSamplingError(&build_options, std::stod(argv[++i]));
      } else if (arg == "--fixed-rank" && hasValue(i, argc)) {
        compression_options.fixed_rank = std::stoi(argv[++i]);
        compression_options.rank_selection = adasdf::RankSelectionMode::FixedRank;
      } else if (arg == "--min-rank" && hasValue(i, argc)) {
        compression_options.min_rank = std::stoi(argv[++i]);
      } else if (arg == "--max-rank" && hasValue(i, argc)) {
        compression_options.max_rank = std::stoi(argv[++i]);
      } else if (arg == "--keep-near-surface-dense") {
        compression_options.always_keep_near_surface_blocks_dense = true;
      } else if (arg == "--report" && hasValue(i, argc)) {
        build_report_path = argv[++i];
      } else if (arg == "--compression-report" && hasValue(i, argc)) {
        compression_report_path = argv[++i];
      } else if (arg == "--quality-report" && hasValue(i, argc)) {
        quality_report_path = argv[++i];
      } else if (arg == "--strict-json" && hasValue(i, argc)) {
        strict_json_path = argv[++i];
      } else if (arg == "--case-id" && hasValue(i, argc)) {
        case_id = argv[++i];
      } else if (arg == "--verbose") {
        build_options.verbose = true;
        compression_options.verbose = true;
      } else if (!arg.empty() && arg[0] == '-') {
        std::cerr << "Unknown or incomplete option: " << arg << "\n";
        usage();
        return 1;
      } else if (input.empty()) {
        input = arg;
      } else if (output.empty()) {
        output = arg;
      } else {
        std::cerr << "Unexpected positional argument: " << arg << "\n";
        usage();
        return 1;
      }
    }

    if (input.empty() || output.empty()) {
      usage();
      write_strict(false, "failed", "missing input or output path");
      return 1;
    }
    if (!std::filesystem::exists(input)) {
      std::cerr << "adasdf_build_compressed_sdf: input STL does not exist: "
                << input.string() << "\n";
      write_strict(false, "failed", "input STL does not exist");
      return 1;
    }

    adasdf::AdaptiveBlockSDFBuildReport build_report;
    auto dense_model = adasdf::AdaptiveBlockSDFBuilder::fromSTL(
        input.string(),
        build_options,
        &build_report);
    if (!build_report_path.empty()) {
      adasdf::AdaptiveBlockSDFBuildReportWriter::writeMarkdown(
          build_report_path.string(),
          build_report);
    }
    if (!dense_model) {
      std::cerr << "adasdf_build_compressed_sdf: adaptive build failed: "
                << build_report.error_message << "\n";
      write_strict(false, "failed", build_report.error_message);
      return build_options.signed_distance &&
                     build_options.require_watertight_for_signed &&
                     !build_report.watertight
                 ? 2
                 : 3;
    }

    auto adaptive =
        std::dynamic_pointer_cast<adasdf::AdaptiveBlockSDFModel>(dense_model);
    if (!adaptive) {
      std::cerr << "adasdf_build_compressed_sdf: adaptive builder returned "
                   "unexpected model type.\n";
      write_strict(false, "failed", "adaptive builder returned unexpected model type");
      return 3;
    }

    adasdf::BlockLowRankCompressionReport compression_report;
    adasdf::CompressedAdaptiveBlockSDF compressed =
        adasdf::BlockLowRankCompressor::compress(
            adaptive->blockSet(),
            compression_options,
            &compression_report);
    if (!compression_report_path.empty()) {
      adasdf::CompressionReportWriter::writeMarkdown(
          compression_report_path.string(),
          compression_report);
    }
    if (!compression_report.success) {
      std::cerr << "adasdf_build_compressed_sdf: compression failed: "
                << compression_report.error_message << "\n";
      write_strict(false, "failed", compression_report.error_message);
      return 3;
    }

    adasdf::CompressedAdaptiveBlockSDFModel compressed_model(std::move(compressed));
    adasdf::CompressionQualityReport quality_report =
        adasdf::CompressionQuality::compare(*adaptive, compressed_model);
    if (!quality_report_path.empty()) {
      adasdf::CompressionReportWriter::writeQualityMarkdown(
          quality_report_path.string(),
          quality_report);
    }

    try {
      adasdf::SDFBinWriter::write(output.string(), compressed_model);
      auto reloaded = adasdf::SDFBinReader::read(output);
      if (!reloaded || !reloaded->isValid() || !reloaded->queryBackendAvailable()) {
        throw std::runtime_error("reloaded compressed model is invalid");
      }
    } catch (const std::exception& exc) {
      std::cerr
          << "adasdf_build_compressed_sdf: write/reload validation failed: "
          << exc.what() << "\n";
      write_strict(false, "failed", exc.what());
      return 4;
    }

    if (!quality_report.success) {
      std::cerr << "adasdf_build_compressed_sdf: quality check failed: "
                << quality_report.error_message << "\n";
      write_strict(false, "failed", quality_report.error_message);
      return 5;
    }

    std::cout << "AdaSDF-CL compressed SDF builder\n";
    std::cout << "Input: " << input.string() << "\n";
    std::cout << "Output: " << output.string() << "\n";
    std::cout << "Format: ADASDF_COMPRESSED_BLOCK_SDFBIN_V1\n";
    std::cout << "Adaptive blocks: " << build_report.block_count << "\n";
    std::cout << "Acceleration: "
              << adasdf::toString(build_report.acceleration_stats.acceleration)
              << "\n";
    std::cout << "Used BVH: " << (build_report.used_bvh ? "yes" : "no")
              << "\n";
    std::cout << "Threads requested: "
              << build_report.acceleration_stats.threads_requested << "\n";
    std::cout << "Threads used: " << build_report.threads_used << "\n";
    std::cout << "BVH build time ms: "
              << build_report.acceleration_stats.bvh_build_time_ms << "\n";
    std::cout << "Sampling time ms: " << build_report.sampling_time_ms
              << "\n";
    std::cout << "BVH nodes: "
              << build_report.acceleration_stats.bvh_node_count << "\n";
    std::cout << "BVH leaves: "
              << build_report.acceleration_stats.bvh_leaf_count << "\n";
    std::cout << "Brute reference time ms: "
              << build_report.acceleration_stats.brute_reference_time_ms
              << "\n";
    std::cout << "Speedup vs brute reference: "
              << build_report.acceleration_stats.speedup_vs_bruteforce
              << "\n";
    std::cout << "Sampling mode: " << samplingModeName(build_options) << "\n";
    if (build_options.hierarchical_sampling.enable_hierarchical_sampling) {
      std::cout << "Hierarchical exact blocks: "
                << build_report.hierarchical_sampling.exact_block_count
                << "\n";
      std::cout << "Hierarchical predicted blocks: "
                << build_report.hierarchical_sampling.predicted_block_count
                << "\n";
      std::cout << "Hierarchical accepted predictions: "
                << build_report
                       .hierarchical_sampling.accepted_prediction_block_count
                << "\n";
      std::cout << "Hierarchical fallback exact blocks: "
                << build_report.hierarchical_sampling.fallback_exact_block_count
                << "\n";
      std::cout << "Hierarchical exact sample count: "
                << build_report.hierarchical_sampling.exact_sample_count
                << "\n";
      std::cout << "Hierarchical predicted sample count: "
                << build_report.hierarchical_sampling.predicted_sample_count
                << "\n";
      std::cout << "Hierarchical quality check samples: "
                << build_report.hierarchical_sampling.quality_check_sample_count
                << "\n";
      std::cout << "Hierarchical speedup estimate: "
                << build_report.hierarchical_sampling.speedup_vs_exact_estimate
                << "\n";
    }
    if (build_options.contact_band_sampling.enable_contact_band_sampling) {
      std::cout << "Contact-band blocks: "
                << build_report.contact_band_sampling.contact_band_block_count
                << "\n";
      std::cout << "Contact-band far-field blocks: "
                << build_report.contact_band_sampling.far_field_block_count
                << "\n";
      std::cout << "Contact-band exact nodes: "
                << build_report.contact_band_sampling.exact_node_count << "\n";
      std::cout << "Contact-band predicted nodes: "
                << build_report.contact_band_sampling.predicted_node_count
                << "\n";
      std::cout << "Contact-band exact node ratio: "
                << build_report.contact_band_sampling.exact_node_ratio << "\n";
      std::cout << "Contact-band sign query reduction ratio: "
                << build_report.contact_band_sampling.sign_query_reduction_ratio
                << "\n";
    }
    std::cout << "Matrix-SVD blocks: "
              << compression_report.compressed_block_count << "\n";
    std::cout << "Dense fallback blocks: "
              << compression_report.dense_fallback_block_count << "\n";
    std::cout << "Compression ratio: "
              << compression_report.compression_ratio << "\n";
    std::cout << "Max abs error: "
              << compression_report.global_max_abs_error << "\n";
    std::cout << "Quality samples: " << quality_report.sample_count << "\n";
    std::cout << "Reload validation: success\n";
    std::cout << "Tucker/HOSVD compression: planned / not implemented\n";
    std::cout
        << "Surrogate recommendation: use adasdf_recommend_build "
           "(implemented in v1.8.0-alpha)\n";
    std::cout << "GPU-native compressed query: planned\n";
    if (!build_report_path.empty()) {
      std::cout << "Build report: " << build_report_path.string() << "\n";
    }
    if (!compression_report_path.empty()) {
      std::cout << "Compression report: "
                << compression_report_path.string() << "\n";
    }
    if (!quality_report_path.empty()) {
      std::cout << "Quality report: " << quality_report_path.string() << "\n";
    }
    write_strict(
        true,
        "ok",
        "",
        {
            {"triangle_count",
             static_cast<double>(build_report.diagnostics.triangle_count)},
            {"vertex_count",
             static_cast<double>(build_report.diagnostics.vertex_count)},
            {"readiness_score",
             static_cast<double>(build_report.readiness.score)},
            {"build_time_ms", build_report.build_time_ms},
            {"memory_bytes", static_cast<double>(build_report.memory_bytes)},
            {"compressed_memory_bytes",
             static_cast<double>(compression_report.compressed_memory_bytes)},
            {"compression_ratio", compression_report.compression_ratio},
            {"hierarchical_exact_blocks",
             static_cast<double>(
                 build_report.hierarchical_sampling.exact_block_count)},
            {"hierarchical_predicted_blocks",
             static_cast<double>(
                 build_report.hierarchical_sampling.predicted_block_count)},
            {"hierarchical_fallback_exact_blocks",
             static_cast<double>(
                 build_report.hierarchical_sampling.fallback_exact_block_count)},
            {"hierarchical_speedup_estimate",
             build_report.hierarchical_sampling.speedup_vs_exact_estimate},
            {"contact_band_blocks",
             static_cast<double>(
                 build_report.contact_band_sampling.contact_band_block_count)},
            {"contact_band_exact_nodes",
             static_cast<double>(
                 build_report.contact_band_sampling.exact_node_count)},
            {"contact_band_predicted_nodes",
             static_cast<double>(
                 build_report.contact_band_sampling.predicted_node_count)},
            {"contact_band_exact_node_ratio",
             build_report.contact_band_sampling.exact_node_ratio},
            {"contact_band_sign_query_reduction_ratio",
             build_report.contact_band_sampling.sign_query_reduction_ratio},
            {"max_abs_error", compression_report.global_max_abs_error},
            {"mean_abs_error", compression_report.global_mean_abs_error},
            {"rms_error", compression_report.global_rms_error},
            {"p95_error", compression_report.global_p95_abs_error},
            {"sign_mismatch_count",
             static_cast<double>(compression_report.sign_mismatch_count)},
            {"near_surface_sign_mismatch_count",
             static_cast<double>(
                 compression_report.near_surface_sign_mismatch_count)}});
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_build_compressed_sdf failed: " << exc.what() << "\n";
    return 3;
  }
}
