#include "adasdf/contract/JsonContractWriter.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "adasdf/report/StrictJsonWriter.h"

namespace adasdf {

std::string JsonContractWriter::generatedAtUtc() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t time = std::chrono::system_clock::to_time_t(now);
  std::tm utc{};
#if defined(_WIN32)
  gmtime_s(&utc, &time);
#else
  gmtime_r(&time, &utc);
#endif
  std::ostringstream out;
  out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
  return out.str();
}

std::string JsonContractWriter::escape(const std::string& text) {
  return StrictJsonWriter::escape(text);
}

std::string JsonContractWriter::quote(const std::string& text) {
  return "\"" + escape(text) + "\"";
}

std::string JsonContractWriter::boolean(bool value) {
  return value ? "true" : "false";
}

std::string JsonContractWriter::number(double value) {
  std::ostringstream out;
  out << std::setprecision(17) << value;
  return out.str();
}

std::string JsonContractWriter::integer(std::size_t value) {
  return std::to_string(value);
}

std::string JsonContractWriter::integerSigned(long long value) {
  return std::to_string(value);
}

std::string JsonContractWriter::vec3(const Vector3& value) {
  return "{\"x\":" + number(value.x) + ",\"y\":" + number(value.y) +
         ",\"z\":" + number(value.z) + "}";
}

std::string JsonContractWriter::aabb(const AABB& value) {
  return "{\"valid\":" + boolean(value.valid) + ",\"min\":" +
         vec3(value.min) + ",\"max\":" + vec3(value.max) + "}";
}

std::string JsonContractWriter::stringArray(
    const std::vector<std::string>& values) {
  std::string out = "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out += ",";
    }
    out += quote(values[i]);
  }
  out += "]";
  return out;
}

std::string JsonContractWriter::warningsArray(
    const std::vector<Warning>& warnings) {
  std::string out = "[";
  for (std::size_t i = 0; i < warnings.size(); ++i) {
    if (i != 0) {
      out += ",";
    }
    out += "{\"code\":" + quote(warnings[i].code) + ",\"message\":" +
           quote(warnings[i].message) + "}";
  }
  out += "]";
  return out;
}

std::string JsonContractWriter::writeObject(
    const BackendJsonContract& contract) {
  const std::string generated = contract.generated_at_utc.empty()
      ? generatedAtUtc()
      : contract.generated_at_utc;
  std::ostringstream out;
  out << "{\n";
  out << "  \"schema_id\": " << quote(contract.schema_id) << ",\n";
  out << "  \"schema_version\": " << contract.schema_version << ",\n";
  out << "  \"adasdf_version\": " << quote(contract.adasdf_version) << ",\n";
  out << "  \"tool_name\": " << quote(contract.tool_name) << ",\n";
  out << "  \"generated_at_utc\": " << quote(generated) << ",\n";
  out << "  \"status\": " << quote(toString(contract.status)) << ",\n";
  out << "  \"status_code\": " << quote(toString(contract.status_code))
      << ",\n";
  out << "  \"success\": " << boolean(contract.success) << ",\n";
  out << "  \"warnings\": " << warningsArray(contract.warnings);
  for (const auto& field : contract.payload_fields) {
    out << ",\n  \"" << escape(field.first) << "\": " << field.second;
  }
  out << "\n}\n";
  return out.str();
}

}  // namespace adasdf
