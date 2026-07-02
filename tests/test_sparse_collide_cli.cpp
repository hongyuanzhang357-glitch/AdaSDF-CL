#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_BUILD_DENSE_SDF
#define ADASDF_CL_BUILD_DENSE_SDF ""
#endif
#ifndef ADASDF_CL_SPARSE_COLLIDE
#define ADASDF_CL_SPARSE_COLLIDE ""
#endif
#ifndef ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR
#define ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR ""
#endif
#ifndef ADASDF_CL_TEST_SAMPLES_DIR
#define ADASDF_CL_TEST_SAMPLES_DIR ""
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

bool expectedCollisionStatus(int code) {
  return code == 10 || code == (10 << 8);
}

}  // namespace

int main() {
  const std::string build_tool = ADASDF_CL_BUILD_DENSE_SDF;
  const std::string collide_tool = ADASDF_CL_SPARSE_COLLIDE;
  if (build_tool.empty() || collide_tool.empty()) {
    std::cout << "SKIP: sparse collide CLI tools were not built\n";
    return 0;
  }
  std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  const auto model = temp / "sparse_collide_cli_dense.sdfbin";
  const auto stdout_txt = temp / "sparse_collide_cli_stdout.txt";
  const auto clearance_txt = temp / "sparse_collide_cli_clearance.txt";
  const auto report = temp / "sparse_collide_cli_report.md";
  const auto samples =
      std::filesystem::path(ADASDF_CL_TEST_SAMPLES_DIR) / "cube_sparse_samples.csv";
  const auto fixture =
      std::filesystem::path(ADASDF_CL_TEST_MESH_DIAGNOSTICS_DIR) /
      "closed_cube_ascii.stl";

  const std::string build_cmd = executableCommand(build_tool);
  const std::string collide_cmd = executableCommand(collide_tool);
  if (std::system((build_cmd + " " + quote(fixture) + " " + quote(model) +
                   " --resolution 24 > " + quote(temp / "sparse_collide_build.txt"))
                      .c_str()) != 0) {
    return 1;
  }
  const int code = std::system(
      (collide_cmd + " " + quote(model) + " " + quote(samples) +
       " --threshold 0 --early-exit --report " + quote(report) +
       " > " + quote(stdout_txt))
          .c_str());
  if (!expectedCollisionStatus(code) ||
      readFile(stdout_txt).find("Colliding: true") == std::string::npos ||
      !std::filesystem::exists(report)) {
    std::cerr << "collision-only sparse collide CLI failed, code " << code << "\n";
    return 1;
  }
  if (std::system((collide_cmd + " " + quote(model) + " " + quote(samples) +
                   " --mode clearance --threshold 0 > " + quote(clearance_txt))
                      .c_str()) != 0 ||
      readFile(clearance_txt).find("Min effective phi") == std::string::npos) {
    std::cerr << "clearance sparse collide CLI failed\n";
    return 1;
  }
  std::cout << "sparse collide CLI passed\n";
  return 0;
}
