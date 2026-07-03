#include <adasdf/adasdf.h>

#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_solver_contact_candidates model.sdfbin samples.csv "
         "[--threshold value] [--top-k N] [--reduction-radius value] "
         "[--max-contacts N] [--max-contacts-per-link N] "
         "[--max-contacts-per-patch N] [--patch-radius value] "
         "[--normal-cos value] [--min-penetration value] [--with-normal] "
         "[--no-radius] [--out solver_contacts.csv] "
         "[--candidates-out raw_candidates.csv] [--json solver_contacts.json] "
         "[--report solver_contacts.md]\n";
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

double maxPenetration(const adasdf::SolverContactSet& contacts) {
  double value = 0.0;
  for (const adasdf::SolverContact& contact : contacts.contacts) {
    value = std::max(value, contact.penetration_depth);
  }
  return value;
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
    adasdf::SparseSDFQueryOptions query_options;
    query_options.threshold = 1e-3;
    query_options.early_exit = false;
    query_options.compute_normals = true;
    query_options.output_mode = adasdf::SparseQueryOutputMode::PhiAndNormal;
    query_options.include_non_colliding_samples = true;
    query_options.sort_results_by_effective_phi = true;

    adasdf::ContactCandidateOptions candidate_options;
    candidate_options.top_k = 32;
    candidate_options.candidate_threshold = query_options.threshold;
    candidate_options.compute_normals = true;

    adasdf::ContactStabilizationOptions stabilize_options;
    std::filesystem::path out_path;
    std::filesystem::path raw_candidates_path;
    std::filesystem::path json_path;
    std::filesystem::path report_path;

    for (int i = 3; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--threshold" && hasValue(i, argc)) {
        query_options.threshold = std::stod(argv[++i]);
        candidate_options.candidate_threshold = query_options.threshold;
      } else if (arg == "--top-k" && hasValue(i, argc)) {
        candidate_options.top_k = std::stoi(argv[++i]);
      } else if (arg == "--reduction-radius" && hasValue(i, argc)) {
        candidate_options.reduction_radius = std::stod(argv[++i]);
      } else if (arg == "--max-contacts" && hasValue(i, argc)) {
        stabilize_options.budget.max_contacts_total = std::stoi(argv[++i]);
      } else if (arg == "--max-contacts-per-link" && hasValue(i, argc)) {
        stabilize_options.budget.max_contacts_per_link = std::stoi(argv[++i]);
        stabilize_options.budget.enforce_link_budget = true;
      } else if (arg == "--max-contacts-per-patch" && hasValue(i, argc)) {
        stabilize_options.budget.max_contacts_per_patch = std::stoi(argv[++i]);
      } else if (arg == "--patch-radius" && hasValue(i, argc)) {
        stabilize_options.patch_options.spatial_radius = std::stod(argv[++i]);
      } else if (arg == "--normal-cos" && hasValue(i, argc)) {
        stabilize_options.patch_options.normal_cosine_threshold = std::stod(argv[++i]);
      } else if (arg == "--min-penetration" && hasValue(i, argc)) {
        stabilize_options.min_penetration_depth = std::stod(argv[++i]);
      } else if (arg == "--with-normal") {
        query_options.compute_normals = true;
        query_options.output_mode = adasdf::SparseQueryOutputMode::PhiAndNormal;
        candidate_options.compute_normals = true;
      } else if (arg == "--no-normal") {
        query_options.compute_normals = false;
        query_options.output_mode = adasdf::SparseQueryOutputMode::PhiOnly;
        candidate_options.compute_normals = false;
      } else if (arg == "--no-radius") {
        query_options.use_sample_radius = false;
      } else if (arg == "--out" && hasValue(i, argc)) {
        out_path = argv[++i];
      } else if (arg == "--candidates-out" && hasValue(i, argc)) {
        raw_candidates_path = argv[++i];
      } else if (arg == "--json" && hasValue(i, argc)) {
        json_path = argv[++i];
      } else if (arg == "--report" && hasValue(i, argc)) {
        report_path = argv[++i];
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
      std::cerr << "adasdf_solver_contact_candidates: failed to load queryable model\n";
      return 1;
    }
    const auto samples = adasdf::CollisionSampleSetIO::readCSV(samples_path.string());
    if (!samples.success) {
      std::cerr << "adasdf_solver_contact_candidates: failed to read samples: "
                << samples.error_message << "\n";
      return 1;
    }

    const adasdf::SparseSDFQueryResult query =
        adasdf::SparseSDFQuery::query(*model, samples.sample_set, query_options);
    if (!query.success) {
      std::cerr << "adasdf_solver_contact_candidates: sparse query failed: "
                << query.error_message << "\n";
      return 2;
    }
    const adasdf::ContactCandidateReductionResult reduced =
        adasdf::ContactCandidateReducer::reduce(query.samples, candidate_options);
    const adasdf::ContactStabilizationResult stabilized =
        adasdf::ContactStabilizer::stabilize(
            reduced.candidates,
            stabilize_options);
    if (!stabilized.success) {
      std::cerr << "adasdf_solver_contact_candidates: stabilization failed: "
                << stabilized.error_message << "\n";
      return 3;
    }
    const adasdf::SolverContactSet contacts =
        adasdf::SolverContactBuilder::fromCandidates(
            stabilized.stabilized_candidates,
            stabilized.patches);

    std::cout << "AdaSDF-CL solver contact candidates\n";
    std::cout << "Samples: " << query.stats.sample_count << "\n";
    std::cout << "Raw candidates: " << reduced.reduced_count << "\n";
    std::cout << "Patches: " << stabilized.stats.patch_count << "\n";
    std::cout << "Solver contacts: " << contacts.size() << "\n";
    std::cout << "Max penetration: " << maxPenetration(contacts) << "\n";
    std::cout << "Max contacts: " << stabilize_options.budget.max_contacts_total << "\n";
    std::cout << "This exports solver-ready candidates, not solver constraints.\n";
    std::cout << "No impulses or friction forces are computed.\n";
    std::cout << "Status: ok\n";

    std::string error;
    if (!raw_candidates_path.empty() &&
        !adasdf::SparseQueryReportWriter::writeCandidatesCSV(
            raw_candidates_path.string(), reduced, &error)) {
      std::cerr << "adasdf_solver_contact_candidates: failed to write raw candidates: "
                << error << "\n";
      return 3;
    }
    if (!out_path.empty() &&
        !adasdf::SolverContactExporter::writeCSV(
            out_path.string(), contacts, &error)) {
      std::cerr << "adasdf_solver_contact_candidates: failed to write CSV: "
                << error << "\n";
      return 3;
    }
    if (!json_path.empty() &&
        !adasdf::SolverContactExporter::writeJson(
            json_path.string(), contacts, &error)) {
      std::cerr << "adasdf_solver_contact_candidates: failed to write JSON: "
                << error << "\n";
      return 3;
    }
    if (!report_path.empty() &&
        !writeText(report_path, adasdf::SolverContactExporter::toMarkdown(contacts))) {
      std::cerr << "adasdf_solver_contact_candidates: failed to write report\n";
      return 3;
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_solver_contact_candidates failed: "
              << exc.what() << "\n";
    return 3;
  }
}
