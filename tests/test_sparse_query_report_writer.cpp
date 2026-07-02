#include <adasdf/adasdf.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

std::string readFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::ostringstream out;
  out << file.rdbuf();
  return out.str();
}

}  // namespace

int main() {
  adasdf::SparseSDFQueryResult query;
  query.success = true;
  query.colliding = true;
  query.stats.sample_count = 1;
  query.stats.queried_count = 1;
  query.stats.result_count = 1;
  query.stats.min_effective_phi = -0.1;
  adasdf::SparseSDFSampleResult sample;
  sample.sample_id = 7;
  sample.effective_phi = -0.1;
  sample.colliding = true;
  query.samples.push_back(sample);

  if (adasdf::SparseQueryReportWriter::queryToMarkdown(query).find("Colliding") ==
      std::string::npos) {
    std::cerr << "markdown missing Colliding\n";
    return 1;
  }
  if (adasdf::SparseQueryReportWriter::queryToJson(query).find("sample_count") ==
      std::string::npos) {
    std::cerr << "json missing sample_count\n";
    return 1;
  }

  std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  std::string error;
  const auto query_csv = temp / "sparse_query_writer.csv";
  if (!adasdf::SparseQueryReportWriter::writeQueryCSV(
          query_csv.string(), query, &error) ||
      readFile(query_csv).find("effective_phi") == std::string::npos) {
    std::cerr << "query CSV failed\n";
    return 1;
  }

  adasdf::ContactCandidateReductionResult candidates;
  candidates.input_count = 1;
  candidates.threshold_candidate_count = 1;
  candidates.reduced_count = 1;
  adasdf::ContactCandidate candidate;
  candidate.rank = 0;
  candidate.sample_id = 7;
  candidates.candidates.push_back(candidate);
  const auto candidate_csv = temp / "candidate_writer.csv";
  if (!adasdf::SparseQueryReportWriter::writeCandidatesCSV(
          candidate_csv.string(), candidates, &error) ||
      readFile(candidate_csv).find("rank") == std::string::npos) {
    std::cerr << "candidate CSV failed\n";
    return 1;
  }
  std::cout << "sparse query report writer passed\n";
  return 0;
}
