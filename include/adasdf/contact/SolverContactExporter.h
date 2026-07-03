#pragma once

#include <string>

#include "adasdf/contact/SolverContact.h"

namespace adasdf {

class SolverContactExporter {
 public:
  static std::string toMarkdown(const SolverContactSet& contacts);
  static std::string toJson(const SolverContactSet& contacts);

  static bool writeCSV(
      const std::string& path,
      const SolverContactSet& contacts,
      std::string* error_message = nullptr);

  static bool writeJson(
      const std::string& path,
      const SolverContactSet& contacts,
      std::string* error_message = nullptr);
};

}  // namespace adasdf
