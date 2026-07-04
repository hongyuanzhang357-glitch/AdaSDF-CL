#include "adasdf/sparse/CollisionSampleSetIO.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace adasdf {
namespace {

std::string trim(const std::string& text) {
  std::size_t first = 0;
  while (first < text.size() &&
         std::isspace(static_cast<unsigned char>(text[first]))) {
    ++first;
  }
  std::size_t last = text.size();
  while (last > first &&
         std::isspace(static_cast<unsigned char>(text[last - 1]))) {
    --last;
  }
  return text.substr(first, last - first);
}

std::string lower(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return text;
}

std::vector<std::string> splitCSV(const std::string& line) {
  std::vector<std::string> fields;
  std::string current;
  bool quoted = false;
  for (std::size_t i = 0; i < line.size(); ++i) {
    const char ch = line[i];
    if (quoted) {
      if (ch == '"' && i + 1 < line.size() && line[i + 1] == '"') {
        current.push_back('"');
        ++i;
      } else if (ch == '"') {
        quoted = false;
      } else {
        current.push_back(ch);
      }
    } else if (ch == '"') {
      quoted = true;
    } else if (ch == ',') {
      fields.push_back(trim(current));
      current.clear();
    } else {
      current.push_back(ch);
    }
  }
  if (quoted) {
    throw std::runtime_error("unterminated quoted CSV field");
  }
  fields.push_back(trim(current));
  return fields;
}

bool looksLikeHeader(const std::vector<std::string>& fields) {
  if (fields.empty()) {
    return false;
  }
  const std::string first = lower(trim(fields.front()));
  if (first == "id" || first == "sample_id") {
    return true;
  }
  for (const std::string& field : fields) {
    const std::string name = lower(trim(field));
    if (name == "x" || name == "radius" || name == "label") {
      return true;
    }
  }
  return false;
}

double parseDouble(
    const std::vector<std::string>& fields,
    std::size_t index,
    double default_value,
    const char* name) {
  if (index >= fields.size() || trim(fields[index]).empty()) {
    return default_value;
  }
  try {
    return std::stod(fields[index]);
  } catch (const std::exception&) {
    throw std::runtime_error(std::string("invalid ") + name + ": " + fields[index]);
  }
}

int parseInt(
    const std::vector<std::string>& fields,
    std::size_t index,
    int default_value,
    const char* name) {
  if (index >= fields.size() || trim(fields[index]).empty()) {
    return default_value;
  }
  try {
    return std::stoi(fields[index]);
  } catch (const std::exception&) {
    throw std::runtime_error(std::string("invalid ") + name + ": " + fields[index]);
  }
}

std::size_t column(
    const std::unordered_map<std::string, std::size_t>& columns,
    const std::string& name,
    std::size_t fallback) {
  const auto iter = columns.find(name);
  return iter == columns.end() ? fallback : iter->second;
}

std::string csvField(const std::string& value) {
  if (value.find_first_of(",\"\n\r") == std::string::npos) {
    return value;
  }
  std::string escaped = "\"";
  for (const char ch : value) {
    if (ch == '"') {
      escaped += "\"\"";
    } else {
      escaped.push_back(ch);
    }
  }
  escaped += "\"";
  return escaped;
}

}  // namespace

CollisionSampleSetReadResult CollisionSampleSetIO::readCSV(
    const std::filesystem::path& path) {
  CollisionSampleSetReadResult result;
  std::ifstream file(path);
  if (!file) {
    result.error_message = "failed to open sample CSV: " + path.string();
    return result;
  }

  std::unordered_map<std::string, std::size_t> columns;
  bool header_seen = false;
  bool first_data_or_header = true;
  int next_auto_id = 0;
  std::string line;
  int line_number = 0;

  while (std::getline(file, line)) {
    ++line_number;
    const std::string trimmed = trim(line);
    if (trimmed.empty() || trimmed.front() == '#') {
      continue;
    }

    std::vector<std::string> fields;
    try {
      fields = splitCSV(line);
    } catch (const std::exception& exc) {
      result.error_message = "line " + std::to_string(line_number) + ": " + exc.what();
      return result;
    }

    if (first_data_or_header) {
      first_data_or_header = false;
      if (looksLikeHeader(fields)) {
        header_seen = true;
        for (std::size_t i = 0; i < fields.size(); ++i) {
          columns[lower(trim(fields[i]))] = i;
        }
        continue;
      }
    }

    try {
      CollisionSample sample;
      const std::size_t id_col = header_seen
          ? column(columns, "sample_id", column(columns, "id", 0))
          : 0;
      const std::size_t x_col = header_seen ? column(columns, "x", 1) : 1;
      const std::size_t y_col = header_seen ? column(columns, "y", 2) : 2;
      const std::size_t z_col = header_seen ? column(columns, "z", 3) : 3;
      sample.sample_id = parseInt(fields, id_col, next_auto_id, "id");
      sample.position = {
          parseDouble(fields, x_col, 0.0, "x"),
          parseDouble(fields, y_col, 0.0, "y"),
          parseDouble(fields, z_col, 0.0, "z")};
      sample.radius = parseDouble(
          fields,
          header_seen ? column(columns, "radius", 4) : 4,
          0.0,
          "radius");
      sample.object_id = parseInt(
          fields,
          header_seen ? column(columns, "object_id", 5) : 5,
          0,
          "object_id");
      sample.link_id = parseInt(
          fields,
          header_seen ? column(columns, "link_id", 6) : 6,
          0,
          "link_id");
      sample.group_id = parseInt(
          fields,
          header_seen ? column(columns, "group_id", 7) : 7,
          0,
          "group_id");
      sample.weight = parseDouble(
          fields,
          header_seen ? column(columns, "weight", 8) : 8,
          1.0,
          "weight");
      const std::size_t label_col =
          header_seen ? column(columns, "label", 9) : 9;
      if (label_col < fields.size()) {
        sample.label = fields[label_col];
      }
      result.sample_set.add(sample);
      next_auto_id = std::max(next_auto_id, sample.sample_id + 1);
    } catch (const std::exception& exc) {
      result.error_message = "line " + std::to_string(line_number) + ": " + exc.what();
      return result;
    }
  }

  result.success = true;
  if (result.sample_set.empty()) {
    result.warnings.push_back("sample CSV contained no samples");
  }
  return result;
}

CollisionSampleSetReadResult CollisionSampleSetIO::readCSV(
    const std::string& path) {
  return readCSV(std::filesystem::path(path));
}

bool CollisionSampleSetIO::writeCSV(
    const std::filesystem::path& path,
    const CollisionSampleSet& sample_set,
    std::string* error_message) {
  try {
    const std::filesystem::path out_path(path);
    if (!out_path.parent_path().empty()) {
      std::filesystem::create_directories(out_path.parent_path());
    }
    std::ofstream file(out_path);
    if (!file) {
      if (error_message) {
        *error_message =
            "failed to open sample CSV for writing: " + path.string();
      }
      return false;
    }
    file << "id,x,y,z,radius,object_id,link_id,group_id,weight,label\n";
    for (const CollisionSample& sample : sample_set.samples) {
      file << sample.sample_id << ","
           << sample.position.x << ","
           << sample.position.y << ","
           << sample.position.z << ","
           << sample.radius << ","
           << sample.object_id << ","
           << sample.link_id << ","
           << sample.group_id << ","
           << sample.weight << ","
           << csvField(sample.label) << "\n";
    }
    return true;
  } catch (const std::exception& exc) {
    if (error_message) {
      *error_message = exc.what();
    }
    return false;
  }
}

bool CollisionSampleSetIO::writeCSV(
    const std::string& path,
    const CollisionSampleSet& sample_set,
    std::string* error_message) {
  return writeCSV(std::filesystem::path(path), sample_set, error_message);
}

}  // namespace adasdf
