#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_build input.stl output.sdfbin [options]\n"
      << "Options:\n"
      << "  --near-surface-error <value>\n"
      << "  --max-memory-mb <value>\n"
      << "  --block-expand-limit-mb <value>\n"
      << "  --max-octree-level <value>\n"
      << "  --max-rank <value>\n"
      << "  --compress\n"
      << "  --no-compress\n"
      << "  --unit <name>\n"
      << "  --use-surrogate\n"
      << "  --top-k <value>\n"
      << "  --verbose\n";
}

bool hasValue(int i, int argc) {
  return i + 1 < argc;
}

void printVec(const char* label, const adasdf::Vector3& v) {
  std::cout << label << v.x << " " << v.y << " " << v.z << "\n";
}

}  // namespace

int main(int argc, char** argv) {
  using namespace adasdf;

  if (argc < 3) {
    usage();
    return argc == 1 ? 0 : 2;
  }

  const std::string input = argv[1];
  const std::string output = argv[2];
  BuildOptions options;
  options.max_octree_level = 5;
  options.max_rank = 8;
  options.near_min_rank = 2;
  options.verbose = false;
  bool surrogate_requested = false;

  for (int i = 3; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--near-surface-error" && hasValue(i, argc)) {
      options.near_surface_error = std::stod(argv[++i]);
      options.tau_near_abs = options.near_surface_error;
    } else if (arg == "--max-memory-mb" && hasValue(i, argc)) {
      options.max_memory_mb = static_cast<std::size_t>(std::stoull(argv[++i]));
      options.memory_limit_mb = static_cast<double>(options.max_memory_mb);
    } else if (arg == "--block-expand-limit-mb" && hasValue(i, argc)) {
      options.block_expand_limit_mb =
          static_cast<std::size_t>(std::stoull(argv[++i]));
      options.block_memory_limit_mb =
          static_cast<double>(options.block_expand_limit_mb);
    } else if (arg == "--max-octree-level" && hasValue(i, argc)) {
      options.max_octree_level = std::stoi(argv[++i]);
    } else if (arg == "--max-rank" && hasValue(i, argc)) {
      options.max_rank = std::stoi(argv[++i]);
    } else if (arg == "--no-compress") {
      options.enable_compression = false;
    } else if (arg == "--compress") {
      options.enable_compression = true;
    } else if (arg == "--unit" && hasValue(i, argc)) {
      options.unit = argv[++i];
    } else if (arg == "--use-surrogate") {
      surrogate_requested = true;
      options.use_surrogate_recommendation = true;
    } else if (arg == "--top-k" && hasValue(i, argc)) {
      options.surrogate_top_k = std::stoi(argv[++i]);
    } else if (arg == "--verbose") {
      options.verbose = true;
    } else {
      std::cerr << "Unknown or incomplete option: " << arg << "\n";
      usage();
      return 2;
    }
  }

  try {
    std::cout << "AdaSDF-CL adaptive builder\n";
    std::cout << "Input mesh: " << input << "\n";
    std::cout << "Output sdfbin: " << output << "\n";
    std::cout << "Near-surface error: " << options.near_surface_error << "\n";
    std::cout << "Max memory: " << options.max_memory_mb << " MB\n";
    std::cout << "Max octree level: " << options.max_octree_level << "\n";
    std::cout << "Compression: "
              << (options.enable_compression ? "enabled" : "disabled") << "\n";

    if (surrogate_requested && !SurrogateRecommender::isAvailable()) {
      std::cout << "Surrogate recommendation: unavailable\n";
      std::cout
          << "Surrogate recommendation backend is not available in this build. "
             "Falling back to user-specified BuildOptions.\n";
    } else {
      std::cout << "Surrogate recommendation: "
                << (surrogate_requested ? "enabled" : "disabled") << "\n";
    }

    auto model = AdaptiveSDFBuilder::fromMesh(input, options);
    SDFBinWriter::write(output, *model);
    auto reloaded = SDFBinReader::read(output);

    std::cout << "Build status: success\n";
    printVec("AABB min: ", reloaded->boundingBox().min);
    printVec("AABB max: ", reloaded->boundingBox().max);
    std::cout << "Memory footprint: " << reloaded->memoryFootprintBytes()
              << " bytes\n";
    std::cout << "Reload validation: "
              << (reloaded->isValid() ? "success" : "failed") << "\n";
    return reloaded->isValid() ? 0 : 1;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_build failed: " << exc.what() << "\n";
    return 1;
  }
}
