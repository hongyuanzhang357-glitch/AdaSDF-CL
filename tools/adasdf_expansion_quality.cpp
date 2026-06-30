#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_expansion_quality model.sdfbin --expansion global|block "
         "[--global-resolution 64] [--block-resolution 32] [--blocks all|0,1,2] "
         "[--samples 10000] [--near-surface-band 1e-3] [--sign-epsilon 1e-9] "
         "[--padding value] [--out quality.csv]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

std::vector<std::string> splitList(const std::string& text) {
  std::vector<std::string> values;
  std::string current;
  for (const char ch : text) {
    if (ch == ',') {
      if (!current.empty()) {
        values.push_back(current);
      }
      current.clear();
    } else {
      current.push_back(ch);
    }
  }
  if (!current.empty()) {
    values.push_back(current);
  }
  return values;
}

adasdf::QueryExpansionMode parseExpansion(const std::string& text) {
  if (text == "global") {
    return adasdf::QueryExpansionMode::Global;
  }
  if (text == "block") {
    return adasdf::QueryExpansionMode::Block;
  }
  throw std::runtime_error("expansion must be global or block");
}

adasdf::BlockSelection parseBlocks(const std::string& text) {
  if (text.empty() || text == "all") {
    return adasdf::BlockSelection::all();
  }
  std::vector<int> ids;
  for (const std::string& item : splitList(text)) {
    ids.push_back(std::stoi(item));
  }
  return adasdf::BlockSelection::selected(std::move(ids));
}

std::string csvField(const std::string& value) {
  if (value.find_first_of(",\"\n\r") == std::string::npos) {
    return value;
  }
  std::string escaped = "\"";
  for (const char ch : value) {
    if (ch == '"') {
      escaped += "\"\"";
    } else {
      escaped += ch;
    }
  }
  escaped += "\"";
  return escaped;
}

void writeCsv(
    std::ostream& out,
    const std::filesystem::path& model_path,
    adasdf::QueryExpansionMode expansion,
    const adasdf::BlockSelection& blocks,
    int resolution,
    const adasdf::ExpansionQualityReport& report) {
  out << "model,expansion,blocks,resolution,samples,max_abs_error,"
         "mean_abs_error,rms_error,p95_abs_error,sign_mismatch_count,"
         "sign_mismatch_rate,ambiguous_sign_count,ambiguous_sign_rate,"
         "near_surface_sign_mismatch_count,near_surface_sign_mismatch_rate,"
         "fallback_count,fallback_rate,worst_point_id\n";
  out << csvField(model_path.string()) << ","
      << adasdf::toString(expansion) << ","
      << csvField(adasdf::blockSelectionString(blocks)) << ","
      << resolution << ","
      << report.num_samples << ","
      << std::setprecision(10)
      << report.max_abs_error << ","
      << report.mean_abs_error << ","
      << report.rms_error << ","
      << report.p95_abs_error << ","
      << report.sign_mismatch_count << ","
      << report.sign_mismatch_rate << ","
      << report.ambiguous_sign_count << ","
      << report.ambiguous_sign_rate << ","
      << report.near_surface_sign_mismatch_count << ","
      << report.near_surface_sign_mismatch_rate << ","
      << report.fallback_count << ","
      << report.fallback_rate << ","
      << report.worst_point_id << "\n";
}

void printPoint(const char* label, const adasdf::Vector3& p) {
  std::cout << label << p.x << " " << p.y << " " << p.z << "\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1) {
      usage();
      return 0;
    }

    const std::filesystem::path model_path = argv[1];
    adasdf::QueryExpansionMode expansion = adasdf::QueryExpansionMode::Global;
    adasdf::BlockSelection blocks = adasdf::BlockSelection::all();
    int global_resolution = 64;
    int block_resolution = 32;
    int samples = 10000;
    double near_surface_band = 1e-3;
    double sign_epsilon = 1e-9;
    double padding = 0.0;
    std::filesystem::path output_path;

    for (int i = 2; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--expansion" && hasValue(i, argc)) {
        expansion = parseExpansion(argv[++i]);
      } else if (arg == "--global-resolution" && hasValue(i, argc)) {
        global_resolution = std::stoi(argv[++i]);
      } else if (arg == "--block-resolution" && hasValue(i, argc)) {
        block_resolution = std::stoi(argv[++i]);
      } else if (arg == "--blocks" && hasValue(i, argc)) {
        blocks = parseBlocks(argv[++i]);
      } else if (arg == "--samples" && hasValue(i, argc)) {
        samples = std::stoi(argv[++i]);
      } else if (arg == "--near-surface-band" && hasValue(i, argc)) {
        near_surface_band = std::stod(argv[++i]);
      } else if (arg == "--sign-epsilon" && hasValue(i, argc)) {
        sign_epsilon = std::stod(argv[++i]);
      } else if (arg == "--padding" && hasValue(i, argc)) {
        padding = std::stod(argv[++i]);
      } else if (arg == "--out" && hasValue(i, argc)) {
        output_path = argv[++i];
      } else if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else {
        std::cerr << "Unknown or incomplete option: " << arg << "\n";
        usage();
        return 2;
      }
    }

    if (!std::filesystem::exists(model_path)) {
      std::cerr << "adasdf_expansion_quality: file does not exist: "
                << model_path.string() << "\n";
      return 2;
    }

    const auto model = adasdf::SDFBinReader::read(model_path);
    if (!model || !model->isValid()) {
      std::cerr << "adasdf_expansion_quality: model is invalid\n";
      return 1;
    }
    if (!model->queryBackendAvailable()) {
      std::cerr
          << "adasdf_expansion_quality: model loaded but no direct query backend "
          << "is available. Rebuild with the demo or existing-core query bridge.\n";
      return 1;
    }

    adasdf::ExpansionOptions expansion_options;
    expansion_options.expansion = expansion;
    expansion_options.block_selection = blocks;
    expansion_options.global_resolution = global_resolution;
    expansion_options.block_resolution = block_resolution;
    expansion_options.padding = padding;
    expansion_options.near_surface_band = near_surface_band;
    expansion_options.sign_epsilon = sign_epsilon;
    expansion_options.enable_quality_audit = true;
    expansion_options.audit_sample_count = samples;

    const adasdf::ExpandedSDF expanded =
        adasdf::SDFExpander::expand(*model, expansion_options);

    adasdf::ExpansionQualityOptions quality_options;
    quality_options.num_samples = samples;
    quality_options.near_surface_band = near_surface_band;
    quality_options.sign_epsilon = sign_epsilon;
    const adasdf::ExpansionQualityReport report =
        adasdf::ExpansionQuality::compareAgainstDirect(
            *model,
            expanded,
            quality_options);

    const int resolution =
        expansion == adasdf::QueryExpansionMode::Global ? global_resolution
                                                        : block_resolution;

    std::cout << "AdaSDF-CL expansion quality audit\n";
    std::cout << "Model: " << model_path.string() << "\n";
    std::cout << "Expansion: " << adasdf::toString(expansion) << "\n";
    std::cout << "Blocks: " << adasdf::blockSelectionString(blocks) << "\n";
    std::cout << "Resolution: " << resolution << "\n";
    std::cout << "Samples: " << report.num_samples << "\n";
    std::cout << "Finite samples: " << report.num_finite_samples << "\n";
    std::cout << "Max abs error: " << report.max_abs_error << "\n";
    std::cout << "Mean abs error: " << report.mean_abs_error << "\n";
    std::cout << "RMS error: " << report.rms_error << "\n";
    std::cout << "P95 abs error: " << report.p95_abs_error << "\n";
    std::cout << "Sign mismatch count: " << report.sign_mismatch_count << "\n";
    std::cout << "Sign mismatch rate: " << report.sign_mismatch_rate << "\n";
    std::cout << "Ambiguous sign count: " << report.ambiguous_sign_count << "\n";
    std::cout << "Ambiguous sign rate: " << report.ambiguous_sign_rate << "\n";
    std::cout << "Near-surface samples: "
              << report.near_surface_sample_count << "\n";
    std::cout << "Near-surface sign mismatch count: "
              << report.near_surface_sign_mismatch_count << "\n";
    std::cout << "Near-surface sign mismatch rate: "
              << report.near_surface_sign_mismatch_rate << "\n";
    std::cout << "Fallback count: " << report.fallback_count << "\n";
    std::cout << "Fallback rate: " << report.fallback_rate << "\n";
    std::cout << "Worst point id: " << report.worst_point_id << "\n";
    printPoint("Worst point: ", report.worst_point);
    std::cout << "Worst direct phi: " << report.worst_direct_phi << "\n";
    std::cout << "Worst expanded phi: " << report.worst_expanded_phi << "\n";
    std::cout << "Direct backend: " << report.direct_backend << "\n";
    std::cout << "Expanded backend: " << report.expanded_backend << "\n";
    std::cout << "Status: ok\n";

    if (!output_path.empty()) {
      if (!output_path.parent_path().empty()) {
        std::filesystem::create_directories(output_path.parent_path());
      }
      std::ofstream file(output_path);
      if (!file) {
        throw std::runtime_error("failed to open quality CSV output");
      }
      writeCsv(file, model_path, expansion, blocks, resolution, report);
    }

    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_expansion_quality failed: " << exc.what() << "\n";
    return 1;
  }
}
