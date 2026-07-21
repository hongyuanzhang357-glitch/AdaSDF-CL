#include <adasdf/adasdf.h>

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct CliOptions {
  std::filesystem::path sdf;
  std::filesystem::path stl;
  std::filesystem::path mismatch_csv;
  double near_band = 0.01;
  std::size_t surface_samples = 3000;
  std::vector<double> offsets = {-0.004, -0.002, -0.001,
                                 0.001, 0.002, 0.004};
  std::vector<int> local_subgrids = {2, 4};
  std::size_t max_cells = 10000;
  std::filesystem::path json;
  std::filesystem::path csv;
  std::filesystem::path report;
  std::string case_id = "mismatch_cell_forensics";
};

void usage() {
  std::cerr
      << "Usage: adasdf_diagnose_mismatch_cells "
         "--sdf model.sdfbin --stl model.stl "
         "[--mismatch-csv mismatch_samples.csv] [--near-band 0.01] "
         "[--surface-samples 3000] [--offsets a,b,c] "
         "[--local-subgrid 2,4] [--max-cells 10000] "
         "[--json out.json] [--csv out.csv] [--report out.md] "
         "[--case-id id]\n";
}

std::vector<double> parseDoubleList(const std::string& text) {
  std::vector<double> values;
  std::stringstream stream(text);
  std::string item;
  while (std::getline(stream, item, ',')) {
    if (!item.empty()) {
      values.push_back(std::stod(item));
    }
  }
  return values;
}

std::vector<int> parseIntList(const std::string& text) {
  std::vector<int> values;
  std::stringstream stream(text);
  std::string item;
  while (std::getline(stream, item, ',')) {
    if (!item.empty()) {
      values.push_back(std::stoi(item));
    }
  }
  return values;
}

bool parseArgs(int argc, char** argv, CliOptions* options) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto need_value = [&](const char* name) {
      if (i + 1 >= argc) {
        std::cerr << name << " requires a value\n";
        return false;
      }
      return true;
    };
    if (arg == "--sdf" && need_value("--sdf")) {
      options->sdf = argv[++i];
    } else if (arg == "--stl" && need_value("--stl")) {
      options->stl = argv[++i];
    } else if (arg == "--mismatch-csv" && need_value("--mismatch-csv")) {
      options->mismatch_csv = argv[++i];
    } else if (arg == "--near-band" && need_value("--near-band")) {
      options->near_band = std::stod(argv[++i]);
    } else if (arg == "--surface-samples" &&
               need_value("--surface-samples")) {
      options->surface_samples =
          static_cast<std::size_t>(std::stoull(argv[++i]));
    } else if (arg == "--offsets" && need_value("--offsets")) {
      options->offsets = parseDoubleList(argv[++i]);
    } else if (arg == "--local-subgrid" &&
               need_value("--local-subgrid")) {
      options->local_subgrids = parseIntList(argv[++i]);
    } else if (arg == "--max-cells" && need_value("--max-cells")) {
      options->max_cells = static_cast<std::size_t>(std::stoull(argv[++i]));
    } else if (arg == "--json" && need_value("--json")) {
      options->json = argv[++i];
    } else if (arg == "--csv" && need_value("--csv")) {
      options->csv = argv[++i];
    } else if (arg == "--report" && need_value("--report")) {
      options->report = argv[++i];
    } else if (arg == "--case-id" && need_value("--case-id")) {
      options->case_id = argv[++i];
    } else if (arg == "--help" || arg == "-h") {
      usage();
      std::exit(0);
    } else {
      std::cerr << "Unknown argument: " << arg << "\n";
      return false;
    }
  }
  return true;
}

bool validate(const CliOptions& options) {
  if (options.sdf.empty() || options.stl.empty()) {
    std::cerr << "--sdf and --stl are required\n";
    return false;
  }
  if (!std::filesystem::exists(options.sdf)) {
    std::cerr << "SDF file not found: " << options.sdf.string() << "\n";
    return false;
  }
  if (!std::filesystem::exists(options.stl)) {
    std::cerr << "STL file not found: " << options.stl.string() << "\n";
    return false;
  }
  if (!(options.near_band > 0.0) || options.offsets.empty() ||
      options.local_subgrids.empty()) {
    std::cerr << "near-band, offsets, and local-subgrid must be valid\n";
    return false;
  }
  return true;
}

std::vector<std::string> parseCsvLine(const std::string& line) {
  std::vector<std::string> fields;
  std::string field;
  bool in_quotes = false;
  for (std::size_t i = 0; i < line.size(); ++i) {
    const char c = line[i];
    if (c == '"') {
      if (in_quotes && i + 1 < line.size() && line[i + 1] == '"') {
        field.push_back('"');
        ++i;
      } else {
        in_quotes = !in_quotes;
      }
    } else if (c == ',' && !in_quotes) {
      fields.push_back(field);
      field.clear();
    } else {
      field.push_back(c);
    }
  }
  fields.push_back(field);
  return fields;
}

bool truthy(const std::string& value) {
  return value == "true" || value == "1" || value == "yes";
}

double getDouble(
    const std::map<std::string, std::size_t>& header,
    const std::vector<std::string>& row,
    const std::string& key,
    double fallback = 0.0) {
  const auto iter = header.find(key);
  if (iter == header.end() || iter->second >= row.size() ||
      row[iter->second].empty()) {
    return fallback;
  }
  return std::stod(row[iter->second]);
}

int getInt(
    const std::map<std::string, std::size_t>& header,
    const std::vector<std::string>& row,
    const std::string& key,
    int fallback = 0) {
  const auto iter = header.find(key);
  if (iter == header.end() || iter->second >= row.size() ||
      row[iter->second].empty()) {
    return fallback;
  }
  return std::stoi(row[iter->second]);
}

std::string getString(
    const std::map<std::string, std::size_t>& header,
    const std::vector<std::string>& row,
    const std::string& key) {
  const auto iter = header.find(key);
  if (iter == header.end() || iter->second >= row.size()) {
    return "";
  }
  return row[iter->second];
}

std::vector<adasdf::SDFAccuracyAuditSample> readMismatchCsv(
    const std::filesystem::path& path) {
  std::ifstream file(path);
  if (!file) {
    throw std::runtime_error("failed to open mismatch CSV: " + path.string());
  }
  std::string line;
  if (!std::getline(file, line)) {
    return {};
  }
  const std::vector<std::string> header_fields = parseCsvLine(line);
  std::map<std::string, std::size_t> header;
  for (std::size_t i = 0; i < header_fields.size(); ++i) {
    header[header_fields[i]] = i;
  }

  std::vector<adasdf::SDFAccuracyAuditSample> samples;
  std::size_t id = 0;
  while (std::getline(file, line)) {
    if (line.empty()) {
      continue;
    }
    const std::vector<std::string> row = parseCsvLine(line);
    adasdf::SDFAccuracyAuditSample sample;
    sample.sample_id = id++;
    sample.point = {
        getDouble(header, row, "x"),
        getDouble(header, row, "y"),
        getDouble(header, row, "z")};
    sample.offset = getDouble(header, row, "offset");
    sample.reference_phi = getDouble(header, row, "reference_phi");
    sample.sdf_phi = getDouble(header, row, "sdf_phi");
    sample.abs_error = getDouble(header, row, "abs_error");
    sample.reference_sign = getInt(header, row, "reference_sign");
    sample.sdf_sign = getInt(header, row, "sdf_sign");
    const std::string kind =
        getString(header, row, "false_inside_or_false_outside");
    sample.false_inside = kind == "false_inside";
    sample.false_outside = kind == "false_outside";
    sample.sign_mismatch = true;
    sample.nearest_triangle_id = getInt(header, row, "nearest_triangle_id", -1);
    sample.block_id = getInt(header, row, "block_id", -1);
    sample.block_level = getInt(header, row, "block_level", -1);
    sample.block_aabb = {
        {getDouble(header, row, "block_min_x"),
         getDouble(header, row, "block_min_y"),
         getDouble(header, row, "block_min_z")},
        {getDouble(header, row, "block_max_x"),
         getDouble(header, row, "block_max_y"),
         getDouble(header, row, "block_max_z")},
        true};
    sample.local_i = getInt(header, row, "local_i", -1);
    sample.local_j = getInt(header, row, "local_j", -1);
    sample.local_k = getInt(header, row, "local_k", -1);
    sample.local_u = getDouble(header, row, "local_u");
    sample.local_v = getDouble(header, row, "local_v");
    sample.local_w = getDouble(header, row, "local_w");
    sample.corner_sign_pattern =
        getString(header, row, "corner_sign_pattern");
    samples.push_back(sample);
  }
  return samples;
}

std::vector<adasdf::SDFAccuracyAuditSample> generateMismatchSamples(
    const adasdf::SDFModel& model,
    const adasdf::TriangleMesh& mesh,
    const CliOptions& cli,
    const adasdf::BlockProvenanceSet* provenance) {
  adasdf::NearSurfaceSampleOptions sample_options;
  sample_options.surface_sample_count = cli.surface_samples;
  sample_options.offsets = cli.offsets;
  const adasdf::NearSurfaceSampleSet samples =
      adasdf::NearSurfaceSampleGenerator::generate(mesh, sample_options);
  adasdf::SDFAccuracyAuditOptions audit_options;
  audit_options.case_id = cli.case_id;
  audit_options.input_sdf = cli.sdf;
  audit_options.input_stl = cli.stl;
  audit_options.near_band = cli.near_band;
  audit_options.normal_audit = true;
  audit_options.cluster_sign_errors = true;
  audit_options.block_source_report = true;
  audit_options.query_source_report = true;
  audit_options.reference_sign_mode = "ray-majority";
  if (provenance != nullptr) {
    audit_options.use_provenance = true;
    audit_options.block_provenance = provenance;
  }
  const adasdf::SDFAccuracyAuditResult audit =
      adasdf::SDFAccuracyAudit::run(model, mesh, samples, audit_options);
  std::vector<adasdf::SDFAccuracyAuditSample> mismatches;
  for (const adasdf::SDFAccuracyAuditSample& sample : audit.samples) {
    if (sample.sign_mismatch) {
      mismatches.push_back(sample);
    }
  }
  return mismatches;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    CliOptions cli;
    if (!parseArgs(argc, argv, &cli) || !validate(cli)) {
      usage();
      return 2;
    }

    const adasdf::STLReadResult read = adasdf::STLReader::read(cli.stl.string());
    if (!read.success) {
      std::cerr << "failed to read STL: " << read.error_message << "\n";
      return 1;
    }
    std::shared_ptr<adasdf::SDFModel> model =
        adasdf::SDFBinReader::read(cli.sdf);
    if (!model || !model->isValid() || !model->queryBackendAvailable()) {
      std::cerr << "SDF model does not have a CPU query backend\n";
      return 1;
    }
    auto adaptive =
        std::dynamic_pointer_cast<adasdf::AdaptiveBlockSDFModel>(model);
    if (!adaptive) {
      std::cerr << "mismatch cell forensics currently requires an adaptive "
                   "dense SDF model\n";
      return 1;
    }

    adasdf::BlockProvenanceSet provenance;
    std::string provenance_error;
    const bool has_provenance =
        adasdf::BlockProvenanceIO::readSidecarForSDF(
            cli.sdf,
            &provenance,
            &provenance_error);

    std::vector<adasdf::SDFAccuracyAuditSample> mismatches;
    if (!cli.mismatch_csv.empty() &&
        std::filesystem::exists(cli.mismatch_csv)) {
      mismatches = readMismatchCsv(cli.mismatch_csv);
    } else {
      mismatches = generateMismatchSamples(
          *model,
          read.mesh,
          cli,
          has_provenance ? &provenance : nullptr);
    }

    adasdf::MismatchCellForensicsOptions options;
    options.case_id = cli.case_id;
    options.near_band = cli.near_band;
    options.local_subgrids = cli.local_subgrids;
    options.max_cells = cli.max_cells;
    options.provenance = has_provenance ? &provenance : nullptr;

    const adasdf::MismatchCellForensicsResult result =
        adasdf::MismatchCellForensics::run(
            *adaptive,
            read.mesh,
            mismatches,
            options);

    if (!cli.json.empty() &&
        !adasdf::MismatchCellReportWriter::writeJson(cli.json, result)) {
      std::cerr << "failed to write JSON: " << cli.json.string() << "\n";
      return 1;
    }
    if (!cli.csv.empty() &&
        !adasdf::MismatchCellReportWriter::writeCSV(cli.csv, result)) {
      std::cerr << "failed to write CSV: " << cli.csv.string() << "\n";
      return 1;
    }
    if (!cli.report.empty() &&
        !adasdf::MismatchCellReportWriter::writeMarkdown(cli.report, result)) {
      std::cerr << "failed to write report: " << cli.report.string() << "\n";
      return 1;
    }

    std::cout << "mismatch cell forensics completed\n";
    std::cout << "sample_count=" << result.sample_count << "\n";
    std::cout << "sign_mismatch_count=" << result.sign_mismatch_count << "\n";
    std::cout << "corner_pattern_all_positive_count="
              << result.corner_pattern_all_positive_count << "\n";
    std::cout << "corner_pattern_all_negative_count="
              << result.corner_pattern_all_negative_count << "\n";
    std::cout << "corner_pattern_mixed_sign_count="
              << result.corner_pattern_mixed_sign_count << "\n";
    std::cout << "surface_crossing_with_same_sign_corners_count="
              << result.surface_crossing_with_same_sign_corners_count << "\n";
    for (const adasdf::MismatchCellSubgridSummary& summary :
         result.subgrid_summaries) {
      std::cout << "local_subgrid_" << summary.subgrid
                << "_fixed_count=" << summary.fixed_count << "\n";
      std::cout << "local_subgrid_" << summary.subgrid
                << "_remaining_mismatch_count="
                << summary.remaining_mismatch_count << "\n";
    }
    std::cout << "cell_resolution_insufficient_likely="
              << (result.cell_resolution_insufficient_likely ? "true"
                                                              : "false")
              << "\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_diagnose_mismatch_cells failed: " << exc.what()
              << "\n";
    return 1;
  }
}
