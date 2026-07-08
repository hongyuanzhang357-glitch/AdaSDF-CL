#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "adasdf/contract/BackendJsonContract.h"
#include "adasdf/geometry/Transform.h"

namespace adasdf {

class JsonContractWriter {
 public:
  static std::string generatedAtUtc();
  static std::string escape(const std::string& text);
  static std::string quote(const std::string& text);
  static std::string boolean(bool value);
  static std::string number(double value);
  static std::string integer(std::size_t value);
  static std::string integerSigned(long long value);
  static std::string vec3(const Vector3& value);
  static std::string aabb(const AABB& value);
  static std::string stringArray(const std::vector<std::string>& values);
  static std::string warningsArray(const std::vector<Warning>& warnings);
  static std::string writeObject(const BackendJsonContract& contract);
};

}  // namespace adasdf
