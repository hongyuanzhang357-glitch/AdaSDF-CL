#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_WRITE_MANIFEST
#define ADASDF_CL_WRITE_MANIFEST ""
#endif
#ifndef ADASDF_CL_COLLECT_RUN_SUMMARY
#define ADASDF_CL_COLLECT_RUN_SUMMARY ""
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
  const std::string write_tool = ADASDF_CL_WRITE_MANIFEST;
  const std::string collect_tool = ADASDF_CL_COLLECT_RUN_SUMMARY;
  if (write_tool.empty() || collect_tool.empty()) {
    std::cout << "SKIP: strict report CLI tools were not built\n";
    return 0;
  }
  const std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  const auto report_a = temp / "collect_a.json";
  const auto report_b = temp / "collect_b.json";
  const auto list = temp / "collect_list.txt";
  const auto summary = temp / "collect_summary.csv";
  for (const auto& item : {std::make_pair(report_a, "case_a"),
                           std::make_pair(report_b, "case_b")}) {
    const std::string command =
        executableCommand(write_tool) + " --case-id " + item.second +
        " --tool unit --input in.stl --output out.sdfbin --out " +
        quote(item.first);
    if (std::system(command.c_str()) != 0) {
      std::cerr << "failed to create manifest for collect test\n";
      return 1;
    }
  }
  {
    std::ofstream out(list);
    out << report_a.string() << "\n" << report_b.string() << "\n";
  }
  const std::string collect =
      executableCommand(collect_tool) + " --inputs " + quote(list) +
      " --out " + quote(summary);
  if (std::system(collect.c_str()) != 0 ||
      readFile(summary).find("case_b") == std::string::npos) {
    std::cerr << "collect run summary CLI failed\n";
    return 1;
  }
  std::cout << "collect run summary CLI passed\n";
  return 0;
}

