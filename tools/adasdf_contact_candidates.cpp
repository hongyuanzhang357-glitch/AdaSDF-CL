#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_contact_candidates model.sdfbin samples.csv "
         "[--top-k N] [--threshold value] [--reduction-radius value] "
         "[--with-normal] [--no-normal] [--no-radius] [--out candidates.csv] "
         "[--report candidates.md] [--json candidates.json]\n";
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
    adasdf::ContactCandidateOptions candidate_options;
    adasdf::SparseSDFQueryOptions query_options;
    query_options.early_exit = false;
    query_options.compute_normals = true;
    query_options.output_mode = adasdf::SparseQueryOutputMode::PhiAndNormal;
    query_options.include_non_colliding_samples = true;
    query_options.sort_results_by_effective_phi = true;

    std::filesystem::path out_path;
    std::filesystem::path report_path;
    std::filesystem::path json_path;

    for (int i = 3; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--top-k" && hasValue(i, argc)) {
        candidate_options.top_k = std::stoi(argv[++i]);
      } else if (arg == "--threshold" && hasValue(i, argc)) {
        candidate_options.candidate_threshold = std::stod(argv[++i]);
        query_options.threshold = candidate_options.candidate_threshold;
      } else if (arg == "--reduction-radius" && hasValue(i, argc)) {
        candidate_options.reduction_radius = std::stod(argv[++i]);
      } else if (arg == "--with-normal") {
        query_options.compute_normals = true;
        candidate_options.compute_normals = true;
        query_options.output_mode = adasdf::SparseQueryOutputMode::PhiAndNormal;
      } else if (arg == "--no-normal") {
        query_options.compute_normals = false;
        candidate_options.compute_normals = false;
        query_options.output_mode = adasdf::SparseQueryOutputMode::PhiOnly;
      } else if (arg == "--no-radius") {
        query_options.use_sample_radius = false;
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
      std::cerr << "adasdf_contact_candidates: failed to load queryable model\n";
      return 1;
    }
    const auto samples = adasdf::CollisionSampleSetIO::readCSV(samples_path.string());
    if (!samples.success) {
      std::cerr << "adasdf_contact_candidates: failed to read samples: "
                << samples.error_message << "\n";
      return 1;
    }

    const adasdf::SparseSDFQueryResult query =
        adasdf::SparseSDFQuery::query(*model, samples.sample_set, query_options);
    if (!query.success) {
      std::cerr << "adasdf_contact_candidates: sparse query failed: "
                << query.error_message << "\n";
      return 2;
    }
    const adasdf::ContactCandidateReductionResult reduced =
        adasdf::ContactCandidateReducer::reduce(query.samples, candidate_options);

    std::cout << "AdaSDF-CL contact candidates\n";
    std::cout << "Input samples: " << reduced.input_count << "\n";
    std::cout << "Threshold candidates: "
              << reduced.threshold_candidate_count << "\n";
    std::cout << "Reduced candidates: " << reduced.reduced_count << "\n";
    std::cout << "Top-K: " << candidate_options.top_k << "\n";
    std::cout << "Min effective phi: " << query.stats.min_effective_phi << "\n";
    std::cout << "This is not a full contact manifold or solver constraints.\n";
    std::cout << "Status: ok\n";

    std::string error;
    if (!out_path.empty() &&
        !adasdf::SparseQueryReportWriter::writeCandidatesCSV(
            out_path.string(), reduced, &error)) {
      std::cerr << "adasdf_contact_candidates: failed to write CSV: "
                << error << "\n";
      return 2;
    }
    if (!report_path.empty() &&
        !writeText(
            report_path,
            adasdf::SparseQueryReportWriter::candidatesToMarkdown(reduced))) {
      std::cerr << "adasdf_contact_candidates: failed to write report\n";
      return 2;
    }
    if (!json_path.empty() &&
        !writeText(json_path, adasdf::SparseQueryReportWriter::candidatesToJson(reduced))) {
      std::cerr << "adasdf_contact_candidates: failed to write JSON report\n";
      return 2;
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_contact_candidates failed: " << exc.what() << "\n";
    return 2;
  }
}
