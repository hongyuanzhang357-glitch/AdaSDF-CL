#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_BUILD_DENSE_SDF
#define ADASDF_CL_BUILD_DENSE_SDF ""
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
  if (tool.find('\\') == std::string::npos &&
      tool.find('/') == std::string::npos) {
    return ".\\" + tool;
  }
#else
  if (tool.find('/') == std::string::npos &&
      tool.find('\\') == std::string::npos) {
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
    const std::string build_tool = ADASDF_CL_BUILD_DENSE_SDF;
    const std::string info_tool = ADASDF_CL_INFO;
    const std::string query_tool = ADASDF_CL_QUERY;
    const std::string collide_tool = ADASDF_CL_COLLIDE;
    if (build_tool.empty() || info_tool.empty() || query_tool.empty() ||
        collide_tool.empty()) {
      std::cout << "SKIP: dense SDF CLI tools were not built\n";
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
    const auto open_stl = fixture_dir / "open_cube_missing_face_ascii.stl";
    const auto dense_sdfbin = temp / "dense_builder_cli_cube.sdfbin";
    const auto unsigned_sdfbin = temp / "dense_builder_cli_open_unsigned.sdfbin";
    const auto failed_sdfbin = temp / "dense_builder_cli_open_signed.sdfbin";
    const auto report_md = temp / "dense_builder_cli_report.md";
    const auto report_json = temp / "dense_builder_cli_report.json";
    const auto usage_txt = temp / "dense_builder_cli_usage.txt";
    const auto build_txt = temp / "dense_builder_cli_build.txt";
    const auto info_txt = temp / "dense_builder_cli_info.txt";
    const auto query_txt = temp / "dense_builder_cli_query.txt";
    const auto collide_txt = temp / "dense_builder_cli_collide.txt";
    const auto unsigned_txt = temp / "dense_builder_cli_unsigned.txt";
    const auto failed_txt = temp / "dense_builder_cli_failed.txt";

    const std::string build_cmd = executableCommand(build_tool);
    const std::string info_cmd = executableCommand(info_tool);
    const std::string query_cmd = executableCommand(query_tool);
    const std::string collide_cmd = executableCommand(collide_tool);

    if (std::system((build_cmd + " > " + quote(usage_txt)).c_str()) != 0) {
      std::cerr << "dense builder usage command failed\n";
      return 1;
    }
    if (!contains(readFile(usage_txt), "Usage: adasdf_build_dense_sdf")) {
      std::cerr << "dense builder usage output missing\n";
      return 1;
    }

    const std::string build =
        build_cmd + " " + quote(closed_stl) + " " + quote(dense_sdfbin) +
        " --resolution 16 --padding 0.05 --report " + quote(report_md) +
        " --json " + quote(report_json) + " > " + quote(build_txt);
    if (std::system(build.c_str()) != 0) {
      std::cerr << "closed cube dense build failed\n";
      return 1;
    }
    if (!std::filesystem::exists(dense_sdfbin) ||
        !std::filesystem::exists(report_md) ||
        !std::filesystem::exists(report_json) ||
        !contains(readFile(build_txt), "Reload validation: success") ||
        !contains(readFile(report_md), "Dense Grid") ||
        !contains(readFile(report_json), "memory_bytes")) {
      std::cerr << "dense build outputs are incomplete\n";
      return 1;
    }

    if (std::system((info_cmd + " " + quote(dense_sdfbin) + " > " +
                     quote(info_txt))
                        .c_str()) != 0) {
      std::cerr << "adasdf_info failed on dense sdfbin\n";
      return 1;
    }
    const std::string info = readFile(info_txt);
    if (!contains(info, "Format: ADASDF_DENSE_SDFBIN_V1") ||
        !contains(info, "DenseSDF resolution: 16 x 16 x 16")) {
      std::cerr << "dense info output missing format or resolution\n";
      return 1;
    }

    const std::string query =
        query_cmd + " " + quote(dense_sdfbin) + " --point 0.5 0.5 0.5 > " +
        quote(query_txt);
    if (std::system(query.c_str()) != 0 ||
        !contains(readFile(query_txt), "Signed distance:")) {
      std::cerr << "adasdf_query failed on dense sdfbin\n";
      return 1;
    }

    const std::string collide =
        collide_cmd + " " + quote(dense_sdfbin) + " " + quote(dense_sdfbin) +
        " --max-contacts 4 > " + quote(collide_txt);
    if (std::system(collide.c_str()) != 0 ||
        !contains(readFile(collide_txt), "Returned contacts:")) {
      std::cerr << "adasdf_collide failed on dense sdfbin\n";
      return 1;
    }

    const std::string unsigned_build =
        build_cmd + " " + quote(open_stl) + " " + quote(unsigned_sdfbin) +
        " --resolution 16 --unsigned > " + quote(unsigned_txt);
    if (std::system(unsigned_build.c_str()) != 0 ||
        !std::filesystem::exists(unsigned_sdfbin)) {
      std::cerr << "open cube unsigned dense build failed\n";
      return 1;
    }

    const std::string signed_open_build =
        build_cmd + " " + quote(open_stl) + " " + quote(failed_sdfbin) +
        " --resolution 16 > " + quote(failed_txt) + " 2>&1";
    if (std::system(signed_open_build.c_str()) == 0) {
      std::cerr << "open cube signed build unexpectedly succeeded\n";
      return 1;
    }
    if (!contains(readFile(failed_txt), "requires a watertight mesh")) {
      std::cerr << "open cube signed failure reason missing\n";
      return 1;
    }

    std::cout << "dense SDF builder CLI passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_build_dense_sdf_cli failed: " << exc.what() << "\n";
    return 1;
  }
}
