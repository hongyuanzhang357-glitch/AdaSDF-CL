#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_build_demo_adaptive output.sdfbin [options]\n"
      << "Options:\n"
      << "  --shape box                  Shape, default box\n"
      << "  --target-error value         Target near-surface error, default 1e-3\n"
      << "  --memory-mb value            Memory limit, default 64\n"
      << "  --block-memory-mb value      Block expansion memory limit, default 16\n"
      << "  --center x y z               Box center, default 0 0 0\n"
      << "  --half-extent hx hy hz       Box half extent, default 0.5 0.5 0.5\n"
      << "  --unit name                  Unit label, default m\n"
      << "  --use-surrogate              Use demo surrogate recommender\n"
      << "  --top-k value                Surrogate top-k, default 5\n";
}

bool hasValue(int index, int argc, int count = 1) {
  return index + count < argc;
}

void printVec(const char* label, const adasdf::Vector3& v) {
  std::cout << label << v.x << " " << v.y << " " << v.z << "\n";
}

}  // namespace

int main(int argc, char** argv) {
  using namespace adasdf;

  if (argc == 1) {
    usage();
    return 0;
  }
  if (argc < 2) {
    usage();
    return 2;
  }

  const std::filesystem::path output = argv[1];
  DemoAdaptiveBuildRequest request;
  request.use_surrogate = false;

  for (int i = 2; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--shape" && hasValue(i, argc)) {
      request.shape_type = argv[++i];
    } else if (arg == "--target-error" && hasValue(i, argc)) {
      request.target_near_surface_error = std::stod(argv[++i]);
    } else if (arg == "--memory-mb" && hasValue(i, argc)) {
      request.memory_limit_mb = std::stod(argv[++i]);
    } else if (arg == "--block-memory-mb" && hasValue(i, argc)) {
      request.block_expand_limit_mb = std::stod(argv[++i]);
    } else if (arg == "--center" && hasValue(i, argc, 3)) {
      request.center = {
          std::stod(argv[++i]), std::stod(argv[++i]), std::stod(argv[++i])};
    } else if (arg == "--half-extent" && hasValue(i, argc, 3)) {
      request.half_extent = {
          std::stod(argv[++i]), std::stod(argv[++i]), std::stod(argv[++i])};
    } else if (arg == "--unit" && hasValue(i, argc)) {
      request.unit = argv[++i];
    } else if (arg == "--use-surrogate") {
      request.use_surrogate = true;
    } else if (arg == "--top-k" && hasValue(i, argc)) {
      request.top_k = std::stoi(argv[++i]);
    } else if (arg == "--help" || arg == "-h") {
      usage();
      return 0;
    } else {
      std::cerr << "Unknown or incomplete option: " << arg << "\n";
      usage();
      return 2;
    }
  }

  try {
    const DemoAdaptiveBuildResult result = DemoAdaptiveSDFBuilder::build(request);
    SDFBinWriter::write(output.string(), *result.model);
    auto reloaded = SDFBinReader::read(output);

    std::cout << "AdaSDF-CL demo adaptive builder\n";
    std::cout << "Shape: " << request.shape_type << "\n";
    printVec("Center: ", request.center);
    printVec("Half extent: ", request.half_extent);
    std::cout << "Target near-surface error: "
              << request.target_near_surface_error << "\n";
    std::cout << "Memory limit: " << request.memory_limit_mb << " MB\n";
    std::cout << "Block memory limit: "
              << request.block_expand_limit_mb << " MB\n";
    std::cout << "Surrogate: " << DemoSurrogateRecommender::id()
              << " experimental\n";
    std::cout << "Warning: " << result.warning << "\n";
    std::cout << "Octree nodes: " << result.model->octreeNodes().size() << "\n";
    std::cout << "Blocks: " << result.model->blocks().size() << "\n";
    std::cout << "Ranks:";
    for (const DemoBlockInfo& block : result.model->blocks()) {
      std::cout << " " << block.rank;
    }
    std::cout << "\n";
    std::cout << "Output: " << output.string() << "\n";
    std::cout << "Reload validation: "
              << (reloaded && reloaded->isValid() ? "success" : "failed")
              << "\n";
    return reloaded && reloaded->isValid() ? 0 : 1;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_build_demo_adaptive failed: " << exc.what() << "\n";
    return 1;
  }
}
