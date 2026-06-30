#include <adasdf/adasdf.h>

#include <algorithm>
#include <exception>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_query_mode_demo --backend cpu|cuda "
         "--expansion none|global|block [--blocks all|0,1,2] "
         "[--points 100000] [--global-resolution 64] [--block-resolution 32] "
         "[--quality-audit] [--samples 10000] [--near-surface-band 1e-3] "
         "[--sign-epsilon 1e-9]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

std::vector<std::string> splitList(const std::string& text) {
  std::vector<std::string> values;
  std::string current;
  for (const char ch : text) {
    if (ch == ',') {
      if (!current.empty()) {
        values.push_back(current);
      }
      current.clear();
    } else {
      current.push_back(ch);
    }
  }
  if (!current.empty()) {
    values.push_back(current);
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
  throw std::runtime_error("backend must be cpu, cuda, or auto");
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

adasdf::AABB blockDomain(
    const adasdf::SDFModel& model,
    const adasdf::BlockSelection& selection) {
  if (selection.use_all_blocks || model.blockMetadata().empty()) {
    return model.boundingBox();
  }
  adasdf::AABB domain;
  for (const adasdf::SDFBlockMetadata& block : model.blockMetadata()) {
    const bool selected = std::find(
        selection.block_ids.begin(),
        selection.block_ids.end(),
        static_cast<int>(block.block_id)) != selection.block_ids.end();
    if (!selected) {
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
    throw std::runtime_error("selected query-mode demo blocks do not exist");
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
  options.center = 0.5 * (domain.min + domain.max);
  options.half_extent = 0.5 * (domain.max - domain.min);
  options.volume_scale = 0.9;
  options.seed = 2027;
  return adasdf::generateBenchmarkPoints(options);
}

}  // namespace

int main(int argc, char** argv) {
  try {
    adasdf::QueryBackend backend = adasdf::QueryBackend::CPU;
    adasdf::QueryExpansionMode expansion = adasdf::QueryExpansionMode::None;
    adasdf::BlockSelection blocks = adasdf::BlockSelection::all();
    std::size_t point_count = 100000;
    int global_resolution = 64;
    int block_resolution = 32;
    bool quality_audit = false;
    int quality_samples = 10000;
    double near_surface_band = 1e-3;
    double sign_epsilon = 1e-9;

    if (argc == 1) {
      usage();
      return 0;
    }

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--backend" && hasValue(i, argc)) {
        backend = parseBackend(argv[++i]);
      } else if (arg == "--expansion" && hasValue(i, argc)) {
        expansion = parseExpansion(argv[++i]);
      } else if (arg == "--blocks" && hasValue(i, argc)) {
        blocks = parseBlocks(argv[++i]);
      } else if (arg == "--points" && hasValue(i, argc)) {
        point_count = static_cast<std::size_t>(std::stoull(argv[++i]));
      } else if (arg == "--global-resolution" && hasValue(i, argc)) {
        global_resolution = std::stoi(argv[++i]);
      } else if (arg == "--block-resolution" && hasValue(i, argc)) {
        block_resolution = std::stoi(argv[++i]);
      } else if (arg == "--quality-audit") {
        quality_audit = true;
      } else if (arg == "--samples" && hasValue(i, argc)) {
        quality_samples = std::stoi(argv[++i]);
      } else if (arg == "--near-surface-band" && hasValue(i, argc)) {
        near_surface_band = std::stod(argv[++i]);
      } else if (arg == "--sign-epsilon" && hasValue(i, argc)) {
        sign_epsilon = std::stod(argv[++i]);
      } else if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else {
        std::cerr << "Unknown or incomplete option: " << arg << "\n";
        usage();
        return 2;
      }
    }

    if (backend == adasdf::QueryBackend::CUDA &&
        expansion == adasdf::QueryExpansionMode::None) {
      std::cerr
          << "CUDA backend requires pre-expanded SDF data. Use Global or Block expansion.\n";
      return 2;
    }

    adasdf::DemoAdaptiveBuildRequest build_request;
    build_request.use_surrogate = false;
    const auto build = adasdf::DemoAdaptiveSDFBuilder::build(build_request);
    if (!build.model) {
      std::cerr << "failed to build demo adaptive SDF model\n";
      return 1;
    }

    adasdf::QueryModeConfig config;
    config.backend = backend;
    config.expansion = expansion;
    config.block_selection = blocks;
    config.allow_fallback_to_cpu = true;
    config.keep_expanded_data_resident = true;

    adasdf::ExpansionOptions expansion_options;
    expansion_options.expansion = expansion;
    expansion_options.block_selection = blocks;
    expansion_options.global_resolution = global_resolution;
    expansion_options.block_resolution = block_resolution;
    expansion_options.near_surface_band = near_surface_band;
    expansion_options.sign_epsilon = sign_epsilon;
    expansion_options.enable_quality_audit = quality_audit;
    expansion_options.audit_sample_count = quality_samples;

    adasdf::QueryEngine engine(build.model, config, expansion_options);
    const bool prepared = engine.prepare();
    if (!prepared) {
      std::cout << "status: skipped\n";
      std::cout << "reason: CUDA backend unavailable and fallback disabled\n";
      return 0;
    }

    const std::vector<adasdf::Vector3> points =
        makePoints(*build.model, blocks, point_count);
    const adasdf::BatchQueryOutput output = engine.queryBatch(points);
    const adasdf::QueryEngineStats& stats = engine.stats();

    std::cout << "AdaSDF-CL query mode demo\n";
    std::cout << "backend: " << adasdf::toString(backend) << "\n";
    std::cout << "expansion: " << adasdf::toString(expansion) << "\n";
    std::cout << "blocks: " << adasdf::blockSelectionString(blocks) << "\n";
    std::cout << "prepared_backend: " << stats.backend << "\n";
    std::cout << "points: " << output.signed_distances.size() << "\n";
    std::cout << "expanded_memory_bytes: " << stats.expanded_memory_bytes << "\n";
    std::cout << "gpu_resident_memory_bytes: " << stats.gpu_resident_memory_bytes << "\n";
    std::cout << "setup_ms: " << stats.setup_ms << "\n";
    std::cout << "query_kernel_ms: "
              << (backend == adasdf::QueryBackend::CUDA ? stats.query_kernel_ms : 0.0)
              << "\n";
    std::cout << "query_total_ms: " << stats.query_total_ms << "\n";
    std::cout << "fallback_count: " << stats.fallback_count << "\n";
    if (quality_audit && expansion != adasdf::QueryExpansionMode::None) {
      const adasdf::ExpandedSDF expanded =
          adasdf::SDFExpander::expand(*build.model, expansion_options);
      adasdf::ExpansionQualityOptions quality_options;
      quality_options.num_samples = quality_samples;
      quality_options.near_surface_band = near_surface_band;
      quality_options.sign_epsilon = sign_epsilon;
      const adasdf::ExpansionQualityReport report =
          adasdf::ExpansionQuality::compareAgainstDirect(
              *build.model,
              expanded,
              quality_options);
      std::cout << "quality_samples: " << report.num_samples << "\n";
      std::cout << "quality_finite_samples: "
                << report.num_finite_samples << "\n";
      std::cout << "quality_max_abs_error: "
                << report.max_abs_error << "\n";
      std::cout << "quality_mean_abs_error: "
                << report.mean_abs_error << "\n";
      std::cout << "quality_rms_error: " << report.rms_error << "\n";
      std::cout << "quality_p95_abs_error: "
                << report.p95_abs_error << "\n";
      std::cout << "quality_sign_mismatch_rate: "
                << report.sign_mismatch_rate << "\n";
      std::cout << "quality_near_surface_sign_mismatch_rate: "
                << report.near_surface_sign_mismatch_rate << "\n";
      std::cout << "quality_fallback_count: "
                << report.fallback_count << "\n";
    } else if (quality_audit) {
      std::cout << "quality_status: skipped\n";
      std::cout << "quality_reason: expansion none has no ExpandedSDF candidate\n";
    }
    std::cout << "description: " << engine.description() << "\n";
    std::cout << "status: ok\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_query_mode_demo failed: " << exc.what() << "\n";
    return 1;
  }
}
