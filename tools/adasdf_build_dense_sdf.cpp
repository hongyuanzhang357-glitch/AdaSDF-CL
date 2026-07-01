#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_build_dense_sdf input.stl output.sdfbin "
         "[--resolution 64] [--padding 0.05] [--signed|--unsigned] "
         "[--require-watertight] [--allow-open-unsigned] [--auto-clean] "
         "[--report build_report.md] [--json build_report.json] [--verbose]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

void writeReports(
    const adasdf::DenseSDFBuildReport& report,
    const std::filesystem::path& markdown,
    const std::filesystem::path& json) {
  if (!markdown.empty()) {
    adasdf::DenseSDFBuildReportWriter::writeMarkdown(markdown.string(), report);
  }
  if (!json.empty()) {
    adasdf::DenseSDFBuildReportWriter::writeJson(json.string(), report);
  }
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1) {
      usage();
      return 0;
    }

    std::filesystem::path input;
    std::filesystem::path output;
    std::filesystem::path report_path;
    std::filesystem::path json_path;
    adasdf::DenseSDFBuildOptions options;

    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else if (arg == "--resolution" && hasValue(i, argc)) {
        options.resolution = std::stoi(argv[++i]);
      } else if (arg == "--padding" && hasValue(i, argc)) {
        options.padding = std::stod(argv[++i]);
      } else if (arg == "--signed") {
        options.signed_distance = true;
      } else if (arg == "--unsigned") {
        options.signed_distance = false;
      } else if (arg == "--require-watertight") {
        options.require_watertight_for_signed = true;
      } else if (arg == "--allow-open-unsigned") {
        options.signed_distance = false;
        options.require_watertight_for_signed = false;
      } else if (arg == "--auto-clean") {
        options.auto_safe_cleanup = true;
      } else if (arg == "--report" && hasValue(i, argc)) {
        report_path = argv[++i];
      } else if (arg == "--json" && hasValue(i, argc)) {
        json_path = argv[++i];
      } else if (arg == "--verbose") {
        options.verbose = true;
      } else if (!arg.empty() && arg[0] == '-') {
        std::cerr << "Unknown or incomplete option: " << arg << "\n";
        usage();
        return 1;
      } else if (input.empty()) {
        input = arg;
      } else if (output.empty()) {
        output = arg;
      } else {
        std::cerr << "Unexpected positional argument: " << arg << "\n";
        usage();
        return 1;
      }
    }

    if (input.empty() || output.empty()) {
      usage();
      return 1;
    }
    if (!std::filesystem::exists(input)) {
      std::cerr << "adasdf_build_dense_sdf: input STL does not exist: "
                << input.string() << "\n";
      return 1;
    }

    adasdf::DenseSDFBuildReport report;
    auto model = adasdf::DenseSDFBuilder::fromSTL(
        input.string(),
        options,
        &report);
    writeReports(report, report_path, json_path);
    if (!model) {
      std::cerr << "adasdf_build_dense_sdf: build failed: "
                << report.error_message << "\n";
      if (options.signed_distance && options.require_watertight_for_signed &&
          !report.watertight) {
        return 2;
      }
      if (report.error_message.find("Failed to read STL") != std::string::npos) {
        return 1;
      }
      return 3;
    }

    try {
      adasdf::SDFBinWriter::write(output.string(), *model);
      auto reloaded = adasdf::SDFBinReader::read(output);
      if (!reloaded || !reloaded->isValid() || !reloaded->queryBackendAvailable()) {
        throw std::runtime_error("reloaded DenseSDF model is invalid");
      }
    } catch (const std::exception& exc) {
      std::cerr << "adasdf_build_dense_sdf: write/reload validation failed: "
                << exc.what() << "\n";
      return 4;
    }

    std::cout << "AdaSDF-CL dense SDF builder\n";
    std::cout << "Input: " << input.string() << "\n";
    std::cout << "Output: " << output.string() << "\n";
    std::cout << "Resolution: " << report.nx << " x " << report.ny << " x "
              << report.nz << "\n";
    std::cout << "Signed: " << (report.signed_distance ? "yes" : "no") << "\n";
    std::cout << "Watertight: " << (report.watertight ? "yes" : "no") << "\n";
    std::cout << "Triangles: " << report.triangle_count << "\n";
    std::cout << "Build time ms: " << report.build_time_ms << "\n";
    std::cout << "Memory bytes: " << report.memory_bytes << "\n";
    std::cout << "Reload validation: success\n";
    if (!report_path.empty()) {
      std::cout << "Report: " << report_path.string() << "\n";
    }
    if (!json_path.empty()) {
      std::cout << "JSON report: " << json_path.string() << "\n";
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_build_dense_sdf failed: " << exc.what() << "\n";
    return 3;
  }
}
