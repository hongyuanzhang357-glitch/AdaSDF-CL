#include <adasdf/adasdf.h>

#include <filesystem>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_collect_run_summary --inputs reports.txt "
         "--out summary.csv\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 1) {
    usage();
    return 0;
  }

  std::filesystem::path inputs;
  std::filesystem::path out;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      usage();
      return 0;
    } else if (arg == "--inputs" && hasValue(i, argc)) {
      inputs = argv[++i];
    } else if (arg == "--out" && hasValue(i, argc)) {
      out = argv[++i];
    } else {
      std::cerr << "Unknown or incomplete option: " << arg << "\n";
      usage();
      return 1;
    }
  }
  if (inputs.empty() || out.empty()) {
    usage();
    return 1;
  }

  std::string error;
  if (!adasdf::ReportCollector::collectRunSummary(inputs, out, &error)) {
    std::cerr << "adasdf_collect_run_summary: failed: " << error << "\n";
    return 1;
  }
  std::cout << "Run summary CSV: " << out.string() << "\n";
  return 0;
}

