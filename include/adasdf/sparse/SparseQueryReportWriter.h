#pragma once

#include <string>

#include "adasdf/sparse/ContactCandidateReducer.h"
#include "adasdf/sparse/SparseCollisionQuery.h"
#include "adasdf/sparse/SparseSDFQuery.h"

namespace adasdf {

class SparseQueryReportWriter {
 public:
  static std::string queryToMarkdown(const SparseSDFQueryResult& result);
  static std::string queryToJson(const SparseSDFQueryResult& result);
  static bool writeQueryCSV(
      const std::string& path,
      const SparseSDFQueryResult& result,
      std::string* error_message = nullptr);

  static std::string collisionToMarkdown(const SparseCollisionResult& result);
  static std::string collisionToJson(const SparseCollisionResult& result);

  static std::string candidatesToMarkdown(
      const ContactCandidateReductionResult& result);
  static std::string candidatesToJson(
      const ContactCandidateReductionResult& result);
  static bool writeCandidatesCSV(
      const std::string& path,
      const ContactCandidateReductionResult& result,
      std::string* error_message = nullptr);
};

}  // namespace adasdf
