#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_BUILD_ADAPTIVE_SDF
#define ADASDF_CL_BUILD_ADAPTIVE_SDF ""
#endif

#ifndef ADASDF_CL_INFO
#define ADASDF_CL_INFO ""
#endif

#ifndef ADASDF_CL_QUERY
#define ADASDF_CL_QUERY ""
#endif

#ifndef ADASDF_CL_COLLIDE
#define ADASDF_CL_COLLIDE ""
#endif

#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

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

bool contains(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

}  // namespace

int main() {
  try {
    const std::string build_tool = ADASDF_CL_BUILD_ADAPTIVE_SDF;
    const std::string info_tool = ADASDF_CL_INFO;
    const std::string query_tool = ADASDF_CL_QUERY;
    const std::string collide_tool = ADASDF_CL_COLLIDE;
    if (build_tool.empty() || info_tool.empty() || query_tool.empty() ||
        collide_tool.empty()) {
      std::cout << "SKIP: adaptive SDF CLI tools were not built\n";
      return 0;
    }

    std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
    if (temp.empty()) {
      temp = std::filesystem::temp_directory_path();
    }
    std::filesystem::create_directories(temp);

    const auto fixture_dir =
        std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR);
    const auto closed_stl = fixture_dir / "closed_cube_ascii.stl";
    const auto adaptive_sdfbin = temp / "adaptive_builder_cli_cube.sdfbin";
    const auto dryrun_sdfbin = temp / "adaptive_builder_cli_dryrun.sdfbin";
    const auto report_md = temp / "adaptive_builder_cli_report.md";
    const auto report_json = temp / "adaptive_builder_cli_report.json";
    const auto dryrun_report = temp / "adaptive_builder_cli_dryrun.md";
    const auto usage_txt = temp / "adaptive_builder_cli_usage.txt";
    const auto build_txt = temp / "adaptive_builder_cli_build.txt";
    const auto info_txt = temp / "adaptive_builder_cli_info.txt";
    const auto query_txt = temp / "adaptive_builder_cli_query.txt";
    const auto collide_txt = temp / "adaptive_builder_cli_collide.txt";
    const auto dryrun_txt = temp / "adaptive_builder_cli_dryrun.txt";
    const auto low_rank_txt = temp / "adaptive_builder_cli_low_rank.txt";

    const std::string build_cmd = executableCommand(build_tool);
    const std::string info_cmd = executableCommand(info_tool);
    const std::string query_cmd = executableCommand(query_tool);
    const std::string collide_cmd = executableCommand(collide_tool);

    if (std::system((build_cmd + " > " + quote(usage_txt)).c_str()) != 0 ||
        !contains(readFile(usage_txt), "Usage: adasdf_build_adaptive_sdf")) {
      std::cerr << "adaptive builder usage output missing\n";
      return 1;
    }

    const std::string build =
        build_cmd + " " + quote(closed_stl) + " " + quote(adaptive_sdfbin) +
        " --max-level 2 --block-resolution 5 --report " + quote(report_md) +
        " --json " + quote(report_json) + " > " + quote(build_txt);
    if (std::system(build.c_str()) != 0 ||
        !std::filesystem::exists(adaptive_sdfbin) ||
        !std::filesystem::exists(report_md) ||
        !std::filesystem::exists(report_json) ||
        !contains(readFile(build_txt), "Reload validation: success") ||
        !contains(readFile(report_md), "Block Stats") ||
        !contains(readFile(report_json), "block_count")) {
      std::cerr << "closed cube adaptive build outputs are incomplete\n";
      return 1;
    }

    if (std::system((info_cmd + " " + quote(adaptive_sdfbin) + " > " +
                     quote(info_txt)).c_str()) != 0 ||
        !contains(readFile(info_txt), "Format: ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1") ||
        !contains(readFile(info_txt), "AdaptiveBlockSDF block_count:")) {
      std::cerr << "adasdf_info failed on adaptive sdfbin\n";
      return 1;
    }

    if (std::system((query_cmd + " " + quote(adaptive_sdfbin) +
                     " --point 0.5 0.5 0.5 > " + quote(query_txt)).c_str()) != 0 ||
        !contains(readFile(query_txt), "Signed distance:")) {
      std::cerr << "adasdf_query failed on adaptive sdfbin\n";
      return 1;
    }

    if (std::system((collide_cmd + " " + quote(adaptive_sdfbin) + " " +
                     quote(adaptive_sdfbin) + " --max-contacts 4 > " +
                     quote(collide_txt)).c_str()) != 0 ||
        !contains(readFile(collide_txt), "Returned contacts:")) {
      std::cerr << "adasdf_collide failed on adaptive sdfbin\n";
      return 1;
    }

    const std::string dryrun =
        build_cmd + " " + quote(closed_stl) + " " + quote(dryrun_sdfbin) +
        " --max-level 2 --block-resolution 5 --dry-run --report " +
        quote(dryrun_report) + " > " + quote(dryrun_txt);
    if (std::system(dryrun.c_str()) != 0 ||
        std::filesystem::exists(dryrun_sdfbin) ||
        !std::filesystem::exists(dryrun_report) ||
        !contains(readFile(dryrun_txt), "Dry run: yes")) {
      std::cerr << "adaptive dry-run behavior failed\n";
      return 1;
    }

    const std::string low_rank =
        build_cmd + " " + quote(closed_stl) + " " + quote(temp / "low_rank.sdfbin") +
        " --enable-low-rank > " + quote(low_rank_txt) + " 2>&1";
    if (std::system(low_rank.c_str()) == 0 ||
        !contains(readFile(low_rank_txt), "planned for v1.7.0-alpha")) {
      std::cerr << "unsupported low-rank request did not fail clearly\n";
      return 1;
    }

    std::cout << "adaptive SDF builder CLI passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_build_adaptive_sdf_cli failed: " << exc.what() << "\n";
    return 1;
  }
}
