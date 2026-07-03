#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_STABILIZE_CONTACTS
#define ADASDF_CL_STABILIZE_CONTACTS ""
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

}  // namespace

int main() {
  const std::string tool = ADASDF_CL_STABILIZE_CONTACTS;
  if (tool.empty()) {
    std::cout << "SKIP: stabilize contacts CLI was not built\n";
    return 0;
  }
  std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  const auto candidates =
      std::filesystem::path(ADASDF_CL_TEST_SAMPLES_DIR) /
      "cube_dense_contact_candidates.csv";
  const auto out_csv = temp / "stabilized_solver_contacts.csv";
  const auto report = temp / "stabilized_solver_contacts.md";
  const auto stdout_txt = temp / "stabilize_contacts_stdout.txt";
  const std::string command =
      executableCommand(tool) + " " + quote(candidates) +
      " --max-contacts 4 --patch-radius 0.05 --out " + quote(out_csv) +
      " --report " + quote(report) + " > " + quote(stdout_txt);
  if (std::system(command.c_str()) != 0 ||
      !std::filesystem::exists(out_csv) ||
      !std::filesystem::exists(report) ||
      readFile(stdout_txt).find("Output solver contacts") == std::string::npos ||
      readFile(out_csv).find("stable_key") == std::string::npos) {
    std::cerr << "stabilize contacts CLI failed\n";
    return 1;
  }
  std::cout << "stabilize contacts CLI passed\n";
  return 0;
}
