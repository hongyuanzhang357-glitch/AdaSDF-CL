#include "adasdf/report/ReportValidator.h"

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace adasdf {
namespace {

std::string readFile(const std::filesystem::path& path, bool* ok) {
  std::ifstream file(path);
  if (!file) {
    *ok = false;
    return {};
  }
  std::ostringstream out;
  out << file.rdbuf();
  *ok = true;
  return out.str();
}

std::size_t skipWhitespace(const std::string& text, std::size_t pos) {
  while (pos < text.size() &&
         std::isspace(static_cast<unsigned char>(text[pos]))) {
    ++pos;
  }
  return pos;
}

std::size_t findKeyValue(const std::string& text, const std::string& key) {
  const std::string quoted = "\"" + key + "\"";
  std::size_t pos = text.find(quoted);
  if (pos == std::string::npos) {
    return pos;
  }
  pos = text.find(':', pos + quoted.size());
  if (pos == std::string::npos) {
    return pos;
  }
  return skipWhitespace(text, pos + 1);
}

std::size_t parseStringEnd(const std::string& text, std::size_t pos) {
  if (pos >= text.size() || text[pos] != '"') {
    return std::string::npos;
  }
  bool escaped = false;
  for (std::size_t i = pos + 1; i < text.size(); ++i) {
    if (escaped) {
      escaped = false;
      continue;
    }
    if (text[i] == '\\') {
      escaped = true;
      continue;
    }
    if (text[i] == '"') {
      return i;
    }
  }
  return std::string::npos;
}

std::size_t parseObjectEnd(const std::string& text, std::size_t pos) {
  if (pos >= text.size() || text[pos] != '{') {
    return std::string::npos;
  }
  int depth = 0;
  bool in_string = false;
  bool escaped = false;
  for (std::size_t i = pos; i < text.size(); ++i) {
    const char ch = text[i];
    if (in_string) {
      if (escaped) {
        escaped = false;
      } else if (ch == '\\') {
        escaped = true;
      } else if (ch == '"') {
        in_string = false;
      }
      continue;
    }
    if (ch == '"') {
      in_string = true;
    } else if (ch == '{') {
      ++depth;
    } else if (ch == '}') {
      --depth;
      if (depth == 0) {
        return i;
      }
    }
  }
  return std::string::npos;
}

std::string rawJsonValue(const std::string& text, const std::string& key) {
  const std::size_t pos = findKeyValue(text, key);
  if (pos == std::string::npos || pos >= text.size()) {
    return {};
  }
  if (text[pos] == '"') {
    const std::size_t end = parseStringEnd(text, pos);
    return end == std::string::npos ? "" : text.substr(pos, end - pos + 1);
  }
  if (text[pos] == '{') {
    const std::size_t end = parseObjectEnd(text, pos);
    return end == std::string::npos ? "" : text.substr(pos, end - pos + 1);
  }
  std::size_t end = pos;
  while (end < text.size() && text[end] != ',' && text[end] != '\n' &&
         text[end] != '\r' && text[end] != '}') {
    ++end;
  }
  return text.substr(pos, end - pos);
}

std::string unquoteJsonString(const std::string& raw) {
  if (raw.size() < 2 || raw.front() != '"' || raw.back() != '"') {
    return raw;
  }
  std::string out;
  bool escaped = false;
  for (std::size_t i = 1; i + 1 < raw.size(); ++i) {
    const char ch = raw[i];
    if (!escaped && ch == '\\') {
      escaped = true;
      continue;
    }
    if (escaped) {
      switch (ch) {
        case 'n':
          out.push_back('\n');
          break;
        case 'r':
          out.push_back('\r');
          break;
        case 't':
          out.push_back('\t');
          break;
        default:
          out.push_back(ch);
          break;
      }
      escaped = false;
    } else {
      out.push_back(ch);
    }
  }
  return out;
}

bool isNumber(const std::string& raw) {
  if (raw.empty()) {
    return false;
  }
  char* end = nullptr;
  std::strtod(raw.c_str(), &end);
  while (end && *end && std::isspace(static_cast<unsigned char>(*end))) {
    ++end;
  }
  return end && *end == '\0';
}

bool isInteger(const std::string& raw) {
  if (raw.empty()) {
    return false;
  }
  char* end = nullptr;
  std::strtol(raw.c_str(), &end, 10);
  while (end && *end && std::isspace(static_cast<unsigned char>(*end))) {
    ++end;
  }
  return end && *end == '\0';
}

bool isBool(const std::string& raw) {
  return raw == "true" || raw == "false";
}

bool isString(const std::string& raw) {
  return raw.size() >= 2 && raw.front() == '"' && raw.back() == '"';
}

bool isObject(const std::string& raw) {
  return raw.size() >= 2 && raw.front() == '{' && raw.back() == '}';
}

}  // namespace

ReportValidationResult ReportValidator::validateString(const std::string& text) {
  ReportValidationResult result;
  result.readable = true;
  const std::size_t first = skipWhitespace(text, 0);
  if (first >= text.size() || text[first] != '{') {
    result.errors.push_back("report is not a JSON object");
    return result;
  }

  for (const std::string& field : strictReportRequiredFields()) {
    const std::string raw = rawJsonValue(text, field);
    if (raw.empty()) {
      result.errors.push_back("missing required field: " + field);
      continue;
    }
    if (field == "parameters") {
      if (!isObject(raw)) {
        result.errors.push_back("field parameters must be an object");
      }
      result.record.fields[field] = raw;
    } else if (
        field == "success" || field == "cuda_enabled" ||
        field == "cuda_available") {
      if (!isBool(raw)) {
        result.errors.push_back("field " + field + " must be boolean");
      }
      result.record.fields[field] = raw;
    } else if (field == "elapsed_ms") {
      if (!isNumber(raw)) {
        result.errors.push_back("field elapsed_ms must be numeric");
      }
      result.record.fields[field] = raw;
    } else if (field == "cpu_threads") {
      if (!isInteger(raw)) {
        result.errors.push_back("field cpu_threads must be integer");
      }
      result.record.fields[field] = raw;
    } else {
      if (!isString(raw)) {
        result.errors.push_back("field " + field + " must be string");
      }
      result.record.fields[field] = unquoteJsonString(raw);
    }
  }

  for (const std::string& field : strictReportOptionalMetricFields()) {
    const std::string raw = rawJsonValue(text, field);
    if (raw.empty()) {
      continue;
    }
    if (!isNumber(raw)) {
      result.errors.push_back("metric " + field + " must be numeric");
    } else {
      result.record.metrics[field] = raw;
    }
  }

  result.valid = result.errors.empty();
  return result;
}

ReportValidationResult ReportValidator::validateFile(
    const std::filesystem::path& path) {
  bool ok = false;
  const std::string text = readFile(path, &ok);
  if (!ok) {
    ReportValidationResult result;
    result.readable = false;
    result.errors.push_back("failed to read report: " + path.string());
    return result;
  }
  return validateString(text);
}

}  // namespace adasdf
