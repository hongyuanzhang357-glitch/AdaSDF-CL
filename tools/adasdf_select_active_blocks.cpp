#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_select_active_blocks model.sdfbin samples.csv "
         "[--threshold value] [--selection-band value] [--extra-margin value] "
         "[--no-radius] [--no-neighbors] [--no-phi-selection] "
         "[--max-active-blocks N] [--out active_blocks.csv] "
         "[--report report.md] [--json report.json]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

bool writeText(const std::filesystem::path& path, const std::string& text) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream file(path);
  if (!file) {
    return false;
  }
  file << text;
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1 || (argc > 1 && std::string(argv[1]) == "--help")) {
      usage();
      return 0;
    }
    if (argc < 3) {
      usage();
      return 1;
    }

    const std::filesystem::path model_path = argv[1];
    const std::filesystem::path samples_path = argv[2];
    adasdf::ActiveBlockSelectionOptions options;
    std::filesystem::path out_path;
    std::filesystem::path report_path;
    std::filesystem::path json_path;

    for (int i = 3; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--threshold" && hasValue(i, argc)) {
        options.threshold = std::stod(argv[++i]);
      } else if (arg == "--selection-band" && hasValue(i, argc)) {
        options.selection_band = std::stod(argv[++i]);
      } else if (arg == "--extra-margin" && hasValue(i, argc)) {
        options.extra_margin = std::stod(argv[++i]);
      } else if (arg == "--no-radius") {
        options.use_sample_radius = false;
      } else if (arg == "--no-neighbors") {
        options.include_neighbor_blocks = false;
      } else if (arg == "--no-phi-selection") {
        options.query_phi_for_selection = false;
      } else if (arg == "--max-active-blocks" && hasValue(i, argc)) {
        options.max_active_blocks =
            static_cast<std::size_t>(std::stoull(argv[++i]));
      } else if (arg == "--out" && hasValue(i, argc)) {
        out_path = argv[++i];
      } else if (arg == "--report" && hasValue(i, argc)) {
        report_path = argv[++i];
      } else if (arg == "--json" && hasValue(i, argc)) {
        json_path = argv[++i];
      } else if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else {
        std::cerr << "Unknown or incomplete option: " << arg << "\n";
        usage();
        return 1;
      }
    }

    const auto model = adasdf::SDFBinReader::read(model_path);
    if (!model || !model->isValid() || !model->queryBackendAvailable()) {
      std::cerr << "adasdf_select_active_blocks: failed to load queryable model: "
                << model_path.string() << "\n";
      return 1;
    }
    const auto samples =
        adasdf::CollisionSampleSetIO::readCSV(samples_path.string());
    if (!samples.success) {
      std::cerr << "adasdf_select_active_blocks: failed to read samples: "
                << samples.error_message << "\n";
      return 1;
    }

    const adasdf::ActiveBlockSelectionResult result =
        adasdf::ActiveBlockSelector::select(*model, samples.sample_set, options);
    if (!result.success) {
      std::cerr << "adasdf_select_active_blocks: selection failed: "
                << result.error_message << "\n";
      return 2;
    }

    std::cout << "AdaSDF-CL active block selection\n";
    std::cout << "Sample count: " << result.sample_count << "\n";
    std::cout << "Candidate samples: " << result.candidate_sample_count << "\n";
    std::cout << "Active blocks: " << result.block_ids.size() << "\n";
    std::cout << "Threshold: " << result.threshold << "\n";
    std::cout << "Selection band: " << result.selection_band << "\n";
    std::cout << "Extra margin: " << result.extra_margin << "\n";
    std::cout << "Status: ok\n";

    std::string error;
    if (!out_path.empty() &&
        !adasdf::ActiveBlockReportWriter::writeSelectionCSV(
            out_path.string(), result, &error)) {
      std::cerr << "adasdf_select_active_blocks: failed to write CSV: "
                << error << "\n";
      return 2;
    }
    if (!report_path.empty() &&
        !writeText(
            report_path,
            adasdf::ActiveBlockReportWriter::selectionToMarkdown(result))) {
      std::cerr << "adasdf_select_active_blocks: failed to write report\n";
      return 2;
    }
    if (!json_path.empty() &&
        !writeText(
            json_path,
            adasdf::ActiveBlockReportWriter::selectionToJson(result))) {
      std::cerr << "adasdf_select_active_blocks: failed to write JSON report\n";
      return 2;
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_select_active_blocks failed: " << exc.what() << "\n";
    return 2;
  }
}
