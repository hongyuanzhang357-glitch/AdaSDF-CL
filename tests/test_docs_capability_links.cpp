#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifndef ADASDF_CL_SOURCE_DIR
#define ADASDF_CL_SOURCE_DIR ""
#endif

namespace {

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
  const std::filesystem::path source = ADASDF_CL_SOURCE_DIR;
  if (source.empty()) {
    std::cerr << "source directory macro is missing\n";
    return 1;
  }
  const std::string readme = readFile(source / "README.md");
  const std::vector<std::string> docs = {
      "docs/capability_matrix.md",
      "docs/implemented_vs_planned.md",
      "docs/fcl_complement_strategy.md",
      "docs/query_backend_matrix.md",
      "docs/contact_output_matrix.md",
      "docs/public_positioning.md"};

  for (const std::string& doc : docs) {
    if (!std::filesystem::exists(source / doc)) {
      std::cerr << "missing capability doc: " << doc << "\n";
      return 1;
    }
    if (!contains(readme, doc)) {
      std::cerr << "README does not link capability doc: " << doc << "\n";
      return 1;
    }
  }
  if (!contains(readme, "not a drop-in FCL replacement") ||
      !contains(readme, "Capability | Status")) {
    std::cerr << "README capability positioning text is missing\n";
    return 1;
  }

  std::cout << "capability docs links passed\n";
  return 0;
}
