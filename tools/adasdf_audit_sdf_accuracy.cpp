#include <adasdf/adasdf.h>

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct CliOptions {
  std::filesystem::path sdf;
  std::filesystem::path stl;
  std::string mode = "near-surface";
  std::size_t surface_samples = 10000;
  std::vector<double> offsets = {-0.004, -0.002, -0.001, 0.0,
                                 0.001, 0.002, 0.004};
  double near_band = 0.01;
  bool normal_audit = false;
  double normal_eps = 0.0;
  bool cluster_sign_errors = false;
  bool exclude_zero_offset_sign = false;
  bool offset_bin_report = false;
  bool block_source_report = false;
  bool query_source_report = false;
  bool use_provenance = false;
  std::string reference_sign_mode = "ray-majority";
  std::filesystem::path dump_mismatch_samples;
  std::size_t max_dump_samples = 5000;
  std::filesystem::path provenance_json;
  std::filesystem::path query_source_json;
  std::filesystem::path json;
  std::filesystem::path csv;
  std::filesystem::path report;
  std::string case_id;
};

void usage() {
  std::cerr
      << "Usage: adasdf_audit_sdf_accuracy --sdf model.sdfbin --stl model.stl "
         "[--mode near-surface] [--surface-samples N] "
         "[--offsets a,b,c] [--near-band value] [--normal-audit] "
         "[--normal-eps auto|value] [--cluster-sign-errors] "
         "[--dump-mismatch-samples out.csv] [--max-dump-samples N] "
         "[--reference-sign-mode ray-majority|ray-x|ray-y|ray-z|normal-offset|auto] "
         "[--exclude-zero-offset-sign] [--offset-bin-report] "
         "[--block-source-report] [--query-source-report] "
         "[--use-provenance] [--provenance-json path] "
         "[--explain-query-source] [--query-source-json path] "
         "[--json out.json] [--csv out.csv] "
         "[--report out.md] [--case-id id]\n";
}

std::vector<double> parseOffsets(const std::string& text) {
  std::vector<double> values;
  std::stringstream stream(text);
  std::string item;
  while (std::getline(stream, item, ',')) {
    if (item.empty()) {
      continue;
    }
    values.push_back(std::stod(item));
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
    } else if (arg == "--mode" && need_value("--mode")) {
      options->mode = argv[++i];
    } else if (arg == "--surface-samples" &&
               need_value("--surface-samples")) {
      options->surface_samples =
          static_cast<std::size_t>(std::stoull(argv[++i]));
    } else if (arg == "--offsets" && need_value("--offsets")) {
      options->offsets = parseOffsets(argv[++i]);
    } else if (arg == "--near-band" && need_value("--near-band")) {
      options->near_band = std::stod(argv[++i]);
    } else if (arg == "--normal-audit") {
      options->normal_audit = true;
    } else if (arg == "--normal-eps" && need_value("--normal-eps")) {
      const std::string value = argv[++i];
      options->normal_eps = value == "auto" ? 0.0 : std::stod(value);
    } else if (arg == "--cluster-sign-errors") {
      options->cluster_sign_errors = true;
    } else if (arg == "--dump-mismatch-samples" &&
               need_value("--dump-mismatch-samples")) {
      options->dump_mismatch_samples = argv[++i];
      options->cluster_sign_errors = true;
    } else if (arg == "--max-dump-samples" &&
               need_value("--max-dump-samples")) {
      options->max_dump_samples =
          static_cast<std::size_t>(std::stoull(argv[++i]));
    } else if (arg == "--reference-sign-mode" &&
               need_value("--reference-sign-mode")) {
      options->reference_sign_mode = argv[++i];
    } else if (arg == "--exclude-zero-offset-sign") {
      options->exclude_zero_offset_sign = true;
    } else if (arg == "--offset-bin-report") {
      options->offset_bin_report = true;
      options->cluster_sign_errors = true;
    } else if (arg == "--block-source-report") {
      options->block_source_report = true;
      options->cluster_sign_errors = true;
    } else if (arg == "--query-source-report") {
      options->query_source_report = true;
      options->cluster_sign_errors = true;
    } else if (arg == "--explain-query-source") {
      options->query_source_report = true;
      options->cluster_sign_errors = true;
      options->use_provenance = true;
    } else if (arg == "--use-provenance") {
      options->use_provenance = true;
      options->block_source_report = true;
      options->query_source_report = true;
      options->cluster_sign_errors = true;
    } else if (arg == "--provenance-json" && need_value("--provenance-json")) {
      options->provenance_json = argv[++i];
      options->use_provenance = true;
    } else if (arg == "--query-source-json" &&
               need_value("--query-source-json")) {
      options->query_source_json = argv[++i];
      options->query_source_report = true;
      options->cluster_sign_errors = true;
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
  if (options.mode != "near-surface") {
    std::cerr << "Only --mode near-surface is supported\n";
    return false;
  }
  if (options.surface_samples == 0 || options.offsets.empty()) {
    std::cerr << "surface sample count and offsets must be non-empty\n";
    return false;
  }
  if (!(options.near_band > 0.0)) {
    std::cerr << "--near-band must be positive\n";
    return false;
  }
  if (options.reference_sign_mode != "ray-majority" &&
      options.reference_sign_mode != "ray-x" &&
      options.reference_sign_mode != "ray-y" &&
      options.reference_sign_mode != "ray-z" &&
      options.reference_sign_mode != "normal-offset" &&
      options.reference_sign_mode != "auto") {
    std::cerr << "--reference-sign-mode is invalid\n";
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
  return true;
}

void createParent(const std::filesystem::path& path) {
  if (!path.empty() && path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }
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
    const std::shared_ptr<adasdf::SDFModel> model =
        adasdf::SDFBinReader::read(cli.sdf);
    if (!model || !model->isValid() || !model->queryBackendAvailable()) {
      std::cerr << "SDF model does not have an available CPU query backend\n";
      return 1;
    }

    adasdf::NearSurfaceSampleOptions sample_options;
    sample_options.surface_sample_count = cli.surface_samples;
    sample_options.offsets = cli.offsets;
    const adasdf::NearSurfaceSampleSet samples =
        adasdf::NearSurfaceSampleGenerator::generate(read.mesh, sample_options);
    if (samples.samples.empty()) {
      std::cerr << "near-surface sample generation produced no samples\n";
      return 1;
    }

    adasdf::SDFAccuracyAuditOptions audit_options;
    audit_options.case_id = cli.case_id;
    audit_options.input_sdf = cli.sdf;
    audit_options.input_stl = cli.stl;
    audit_options.near_band = cli.near_band;
    audit_options.normal_audit = cli.normal_audit;
    audit_options.normal_eps = cli.normal_eps;
    audit_options.cluster_sign_errors = cli.cluster_sign_errors;
    audit_options.exclude_zero_offset_sign = cli.exclude_zero_offset_sign;
    audit_options.offset_bin_report = cli.offset_bin_report;
    audit_options.block_source_report = cli.block_source_report;
    audit_options.query_source_report = cli.query_source_report;
    adasdf::BlockProvenanceSet provenance;
    if (cli.use_provenance) {
      std::string provenance_error;
      const bool loaded =
          !cli.provenance_json.empty()
              ? adasdf::BlockProvenanceIO::readJson(
                    cli.provenance_json,
                    &provenance,
                    &provenance_error)
              : adasdf::BlockProvenanceIO::readSidecarForSDF(
                    cli.sdf,
                    &provenance,
                    &provenance_error);
      if (!loaded) {
        std::cerr << "failed to read provenance: " << provenance_error
                  << "\n";
        return 1;
      }
      audit_options.use_provenance = true;
      audit_options.block_provenance = &provenance;
      audit_options.block_source_report = true;
      audit_options.query_source_report = true;
    }
    audit_options.reference_sign_mode =
        cli.reference_sign_mode == "auto" ? "ray-majority"
                                          : cli.reference_sign_mode;
    adasdf::SDFAccuracyAuditResult result =
        adasdf::SDFAccuracyAudit::run(
            *model,
            read.mesh,
            samples,
            audit_options);

    if (!cli.json.empty()) {
      createParent(cli.json);
      if (!adasdf::SDFAccuracyReportWriter::writeJson(cli.json, result)) {
        std::cerr << "failed to write JSON: " << cli.json.string() << "\n";
        return 1;
      }
    }
    if (!cli.query_source_json.empty()) {
      createParent(cli.query_source_json);
      if (!adasdf::SDFAccuracyReportWriter::writeJson(
              cli.query_source_json,
              result)) {
        std::cerr << "failed to write query-source JSON: "
                  << cli.query_source_json.string() << "\n";
        return 1;
      }
    }
    if (!cli.csv.empty()) {
      createParent(cli.csv);
      if (!adasdf::SDFAccuracyReportWriter::writeCSV(cli.csv, result)) {
        std::cerr << "failed to write CSV: " << cli.csv.string() << "\n";
        return 1;
      }
    }
    if (!cli.dump_mismatch_samples.empty()) {
      createParent(cli.dump_mismatch_samples);
      if (!adasdf::SDFAccuracyReportWriter::writeMismatchCSV(
              cli.dump_mismatch_samples,
              result,
              cli.max_dump_samples)) {
        std::cerr << "failed to write mismatch CSV: "
                  << cli.dump_mismatch_samples.string() << "\n";
        return 1;
      }
    }
    if (!cli.report.empty()) {
      createParent(cli.report);
      if (!adasdf::SDFAccuracyReportWriter::writeMarkdown(cli.report, result)) {
        std::cerr << "failed to write report: " << cli.report.string() << "\n";
        return 1;
      }
    }

    std::cout << "SDF accuracy audit completed\n";
    std::cout << "near_surface_sample_count="
              << result.near_surface_sample_count << "\n";
    std::cout << "p95_abs_error=" << result.p95_abs_error << "\n";
    std::cout << "sign_mismatch_count=" << result.sign_mismatch_count << "\n";
    std::cout << "full_quality_passed="
              << (result.full_quality_passed ? "true" : "false") << "\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_audit_sdf_accuracy failed: " << exc.what() << "\n";
    return 1;
  }
}
