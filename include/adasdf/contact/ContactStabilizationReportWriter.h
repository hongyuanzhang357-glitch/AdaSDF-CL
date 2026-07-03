#pragma once

#include <string>

#include "adasdf/contact/ContactStabilizer.h"

namespace adasdf {

class ContactStabilizationReportWriter {
 public:
  static std::string toMarkdown(const ContactStabilizationResult& result);
  static std::string toJson(const ContactStabilizationResult& result);
};

}  // namespace adasdf
