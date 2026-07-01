#include "adasdf/recommendation/SurrogateProfile.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace adasdf {
namespace {

std::string trim(const std::string& value) {
  std::size_t first = 0;
  while (first < value.size() &&
         std::isspace(static_cast<unsigned char>(value[first]))) {
    ++first;
  }
  std::size_t last = value.size();
  while (last > first &&
         std::isspace(static_cast<unsigned char>(value[last - 1]))) {
    --last;
  }
  return value.substr(first, last - first);
}

bool parseDouble(const std::string& text, double* value) {
  if (!value) {
    return false;
  }
  try {
    std::size_t consumed = 0;
    const double parsed = std::stod(text, &consumed);
    if (consumed != text.size()) {
      return false;
    }
    *value = parsed;
    return true;
  } catch (const std::exception&) {
    return false;
  }
}

bool parseInt(const std::string& text, int* value) {
  if (!value) {
    return false;
  }
  try {
    std::size_t consumed = 0;
    const int parsed = std::stoi(text, &consumed);
    if (consumed != text.size()) {
      return false;
    }
    *value = parsed;
    return true;
  } catch (const std::exception&) {
    return false;
  }
}

void addWarning(
    SurrogateProfileLoadResult& result,
    const std::string& warning) {
  result.warnings.push_back(warning);
}

}  // namespace

const char* SurrogateProfile::builtInProfileId() {
  return "v1_8_deterministic_default";
}

const char* SurrogateProfile::statusWarning() {
  return "experimental deterministic build recommender profile; not a "
         "universal trained model; not fully trained; not an optimality "
         "guarantee";
}

SurrogateProfileLoadResult SurrogateProfile::defaults() {
  SurrogateProfileLoadResult result;
  result.loaded = true;
  result.warnings.push_back(statusWarning());
  return result;
}

SurrogateProfileLoadResult SurrogateProfile::load(
    const std::string& path,
    const SurrogateEstimatorOptions& fallback) {
  SurrogateProfileLoadResult result;
  result.options = fallback;
  result.loaded = false;
  if (path.empty()) {
    result.loaded = true;
    addWarning(result, statusWarning());
    return result;
  }

  std::ifstream file(path);
  if (!file) {
    addWarning(
        result,
        "failed to load surrogate profile; using built-in defaults: " + path);
    addWarning(result, statusWarning());
    return result;
  }

  std::string line;
  int line_number = 0;
  while (std::getline(file, line)) {
    ++line_number;
    const std::size_t comment = line.find('#');
    if (comment != std::string::npos) {
      line = line.substr(0, comment);
    }
    line = trim(line);
    if (line.empty()) {
      continue;
    }
    const std::size_t eq = line.find('=');
    if (eq == std::string::npos) {
      addWarning(
          result,
          "ignored malformed surrogate profile line " +
              std::to_string(line_number));
      continue;
    }
    const std::string key = trim(line.substr(0, eq));
    const std::string value = trim(line.substr(eq + 1));

    double d = 0.0;
    int i = 0;
    if (key == "memory_safety_factor") {
      if (parseDouble(value, &d) && d > 0.0) {
        result.options.memory_safety_factor = d;
      } else {
        addWarning(result, "invalid memory_safety_factor; keeping default");
      }
    } else if (key == "error_safety_factor") {
      if (parseDouble(value, &d) && d > 0.0) {
        result.options.error_safety_factor = d;
      } else {
        addWarning(result, "invalid error_safety_factor; keeping default");
      }
    } else if (key == "compression_ratio_safety_factor") {
      if (parseDouble(value, &d) && d > 0.0) {
        result.options.compression_ratio_safety_factor = d;
      } else {
        addWarning(
            result,
            "invalid compression_ratio_safety_factor; keeping default");
      }
    } else if (key == "min_max_level") {
      if (parseInt(value, &i) && i >= 0) {
        result.options.min_max_level = i;
      } else {
        addWarning(result, "invalid min_max_level; keeping default");
      }
    } else if (key == "max_max_level") {
      if (parseInt(value, &i) && i >= 0) {
        result.options.max_max_level = i;
      } else {
        addWarning(result, "invalid max_max_level; keeping default");
      }
    } else if (key == "min_block_resolution") {
      if (parseInt(value, &i) && i >= 2) {
        result.options.min_block_resolution = i;
      } else {
        addWarning(result, "invalid min_block_resolution; keeping default");
      }
    } else if (key == "max_block_resolution") {
      if (parseInt(value, &i) && i >= 2) {
        result.options.max_block_resolution = i;
      } else {
        addWarning(result, "invalid max_block_resolution; keeping default");
      }
    } else if (key == "min_rank") {
      if (parseInt(value, &i) && i >= 1) {
        result.options.min_rank = i;
      } else {
        addWarning(result, "invalid min_rank; keeping default");
      }
    } else if (key == "max_rank") {
      if (parseInt(value, &i) && i >= 1) {
        result.options.max_rank = i;
      } else {
        addWarning(result, "invalid max_rank; keeping default");
      }
    } else if (key == "notes") {
      addWarning(result, value);
    } else {
      addWarning(result, "unknown surrogate profile key ignored: " + key);
    }
  }

  if (result.options.max_max_level < result.options.min_max_level) {
    addWarning(
        result,
        "max_max_level was lower than min_max_level; restoring defaults");
    result.options.min_max_level = fallback.min_max_level;
    result.options.max_max_level = fallback.max_max_level;
  }
  if (result.options.max_block_resolution <
      result.options.min_block_resolution) {
    addWarning(
        result,
        "max_block_resolution was lower than min_block_resolution; "
        "restoring defaults");
    result.options.min_block_resolution = fallback.min_block_resolution;
    result.options.max_block_resolution = fallback.max_block_resolution;
  }
  if (result.options.max_rank < result.options.min_rank) {
    addWarning(result, "max_rank was lower than min_rank; restoring defaults");
    result.options.min_rank = fallback.min_rank;
    result.options.max_rank = fallback.max_rank;
  }
  result.loaded = true;
  addWarning(result, statusWarning());
  return result;
}

}  // namespace adasdf
