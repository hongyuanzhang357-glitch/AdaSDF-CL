#include <adasdf/adasdf.h>

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_ADAPTIVE_PREVIEW
#define ADASDF_CL_ADAPTIVE_PREVIEW ""
#endif

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

bool contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

std::string executableCommand(const std::string& tool) {
#ifdef _WIN32
  if (tool.find('\\') == std::string::npos && tool.find('/') == std::string::npos) {
    return ".\\" + tool;
  }
#else
  if (tool.find('/') == std::string::npos && tool.find('\\') == std::string::npos) {
    return "./" + tool;
  }
#endif
  return "\"" + tool + "\"";
}

std::string quote(const std::filesystem::path& path) {
  return "\"" + path.string() + "\"";
}

std::string readFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::ostringstream out;
  out << file.rdbuf();
  return out.str();
}

}  // namespace

int main() {
  try {
    adasdf::AdaptiveSDFBuildOptions options;
    const adasdf::AdaptiveSDFBuildPlan plan =
        adasdf::AdaptiveSDFBuilderPreview::makePlan(options);
    const std::string markdown =
        adasdf::AdaptiveSDFBuilderPreview::planToMarkdown(plan);
    if (!plan.implemented_in_this_version ||
        !contains(markdown, "OctreeRefinement implemented in v1.6.0-alpha") ||
        !contains(markdown, "BlockPartition implemented in v1.6.0-alpha") ||
        !contains(markdown, "LowRankCompression implemented in v1.7.0-alpha") ||
        !contains(markdown, "SurrogateRecommendation implemented in v1.8.0-alpha") ||
        !contains(markdown, "matrix-SVD") ||
        !contains(markdown, "TuckerCompression planned")) {
      std::cerr << "preview v1.8 wording missing\n";
      return 1;
    }

    const std::string preview_tool = ADASDF_CL_ADAPTIVE_PREVIEW;
    if (!preview_tool.empty()) {
      std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
      if (temp.empty()) {
        temp = std::filesystem::temp_directory_path();
      }
      std::filesystem::create_directories(temp);
      const auto fixture_dir =
          std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR);
      const auto out_sdfbin = temp / "preview_should_not_exist.sdfbin";
      const auto out_plan = temp / "preview_v1_6_plan.md";
      const auto out_txt = temp / "preview_v1_6.txt";
      const std::string cmd =
          executableCommand(preview_tool) + " " +
          quote(fixture_dir / "closed_cube_ascii.stl") + " " +
          quote(out_sdfbin) + " --dry-run --plan " + quote(out_plan) +
          " > " + quote(out_txt);
      if (std::system(cmd.c_str()) != 0 ||
          std::filesystem::exists(out_sdfbin) ||
          !contains(readFile(out_txt), "Use adasdf_build_compressed_sdf")) {
        std::cerr << "preview CLI generated output or missed v1.8 guidance\n";
        return 1;
      }
    }
    std::cout << "adaptive builder preview update passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_adaptive_builder_preview_updated failed: "
              << exc.what() << "\n";
    return 1;
  }
}
