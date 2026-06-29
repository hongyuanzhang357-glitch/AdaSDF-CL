#include <adasdf/adasdf.h>

#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>

int main() {
  using namespace adasdf;

  try {
    DemoSurrogateInput input;
    input.shape_type = "box";
    input.target_near_surface_error = 1.0e-3;
    input.memory_limit_mb = 64.0;
    input.block_expand_limit_mb = 16.0;
    input.top_k = 5;

    const auto candidates = DemoSurrogateRecommender::recommend(input);
    if (candidates.empty()) {
      std::cerr << "no demo surrogate candidates\n";
      return 1;
    }

    DemoAdaptiveBuildRequest request;
    request.use_surrogate = true;
    request.target_near_surface_error = input.target_near_surface_error;
    request.memory_limit_mb = input.memory_limit_mb;
    request.block_expand_limit_mb = input.block_expand_limit_mb;
    request.top_k = input.top_k;

    const auto build = DemoAdaptiveSDFBuilder::build(request);
    const Scalar phi = build.model->sampleDistance({0.0, 0.0, 0.0});
    const auto temp_sdfbin =
        std::filesystem::temp_directory_path() /
        "adasdf_v0_9_surrogate_adaptive_demo.sdfbin";
    SDFBinWriter::write(temp_sdfbin.string(), *build.model);
    const auto reloaded = SDFBinReader::read(temp_sdfbin);

    std::cout << "AdaSDF-CL surrogate adaptive demo\n";
    std::cout << "Warning: " << DemoSurrogateRecommender::statusWarning()
              << "\n";
    std::cout << "Candidates: " << candidates.size() << "\n";
    std::cout << "Selected max octree level: "
              << build.candidate.options.max_octree_level << "\n";
    std::cout << "Selected block resolution: "
              << build.candidate.options.base_block_cells << "\n";
    std::cout << "Selected rank: "
              << build.candidate.options.max_rank << "\n";
    std::cout << "Octree nodes: " << build.model->octreeNodes().size() << "\n";
    std::cout << "Blocks: " << build.model->blocks().size() << "\n";
    std::cout << "Signed distance at origin: " << phi << "\n";
    std::cout << "Reloaded: " << (reloaded && reloaded->isValid() ? "true" : "false")
              << "\n";
    return std::isfinite(phi) && reloaded && reloaded->isValid() ? 0 : 1;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_surrogate_adaptive_demo failed: "
              << exc.what() << "\n";
    return 1;
  }
}
