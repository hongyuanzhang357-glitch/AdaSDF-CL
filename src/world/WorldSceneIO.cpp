#include "adasdf/world/WorldSceneIO.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_map>

#include "adasdf/io/SDFBinReader.h"
#include "adasdf/sparse/CollisionSampleSetIO.h"

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

std::vector<std::string> splitCSVLine(const std::string& line) {
  std::vector<std::string> fields;
  std::string current;
  bool quoted = false;
  for (std::size_t i = 0; i < line.size(); ++i) {
    const char ch = line[i];
    if (ch == '"') {
      if (quoted && i + 1 < line.size() && line[i + 1] == '"') {
        current.push_back('"');
        ++i;
      } else {
        quoted = !quoted;
      }
    } else if (ch == ',' && !quoted) {
      fields.push_back(trim(current));
      current.clear();
    } else {
      current.push_back(ch);
    }
  }
  fields.push_back(trim(current));
  return fields;
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
  escaped.push_back('"');
  return escaped;
}

std::unordered_map<std::string, std::size_t> headerMap(
    const std::vector<std::string>& header) {
  std::unordered_map<std::string, std::size_t> result;
  for (std::size_t i = 0; i < header.size(); ++i) {
    result.emplace(header[i], i);
  }
  return result;
}

std::string valueOr(
    const std::vector<std::string>& fields,
    const std::unordered_map<std::string, std::size_t>& header,
    const std::string& name,
    const std::string& fallback = "") {
  const auto iter = header.find(name);
  if (iter == header.end() || iter->second >= fields.size()) {
    return fallback;
  }
  const std::string value = trim(fields[iter->second]);
  return value.empty() ? fallback : value;
}

double doubleValue(
    const std::vector<std::string>& fields,
    const std::unordered_map<std::string, std::size_t>& header,
    const std::string& name,
    double fallback = 0.0) {
  const std::string value = valueOr(fields, header, name);
  return value.empty() ? fallback : std::stod(value);
}

int intValue(
    const std::vector<std::string>& fields,
    const std::unordered_map<std::string, std::size_t>& header,
    const std::string& name,
    int fallback = -1) {
  const std::string value = valueOr(fields, header, name);
  return value.empty() ? fallback : std::stoi(value);
}

bool boolValue(
    const std::vector<std::string>& fields,
    const std::unordered_map<std::string, std::size_t>& header,
    const std::string& name,
    bool fallback = true) {
  const std::string value = valueOr(fields, header, name);
  if (value.empty()) {
    return fallback;
  }
  return value == "1" || value == "true" || value == "TRUE" ||
      value == "yes" || value == "enabled";
}

std::filesystem::path resolvePath(
    const std::filesystem::path& scene_path,
    const std::string& raw) {
  if (raw.empty()) {
    return {};
  }
  std::filesystem::path path;
  try {
    path = std::filesystem::u8path(raw);
  } catch (const std::exception&) {
    path = std::filesystem::path(raw);
  }
  if (path.is_relative()) {
    path = scene_path.parent_path() / path;
  }
  return path.lexically_normal();
}

std::string pathToUtf8(const std::filesystem::path& path) {
  return path.u8string();
}

}  // namespace

WorldSceneReadResult WorldSceneIO::readCSV(
    const std::filesystem::path& path,
    const WorldSceneReadOptions& options) {
  WorldSceneReadResult result;
  std::ifstream file(path);
  if (!file) {
    result.error_message = "failed to open world scene CSV: " + path.string();
    return result;
  }

  std::string line;
  std::vector<std::string> header;
  std::size_t line_number = 0;
  while (std::getline(file, line)) {
    ++line_number;
    const std::string stripped = trim(line);
    if (stripped.empty() || stripped[0] == '#') {
      continue;
    }
    header = splitCSVLine(stripped);
    break;
  }
  if (header.empty()) {
    result.error_message = "world scene CSV has no header";
    return result;
  }

  const auto map = headerMap(header);
  while (std::getline(file, line)) {
    ++line_number;
    const std::string stripped = trim(line);
    if (stripped.empty() || stripped[0] == '#') {
      continue;
    }
    const std::vector<std::string> fields = splitCSVLine(stripped);
    try {
      WorldObject object;
      object.object_id = intValue(fields, map, "object_id", -1);
      object.name = valueOr(fields, map, "name");
      object.model_path = resolvePath(path, valueOr(fields, map, "model_path"));
      object.samples_path = resolvePath(path, valueOr(fields, map, "samples_path"));
      object.transform = WorldTransform::fromQuaternionTranslation(
          WorldQuaternion{
              doubleValue(fields, map, "qw", 1.0),
              doubleValue(fields, map, "qx", 0.0),
              doubleValue(fields, map, "qy", 0.0),
              doubleValue(fields, map, "qz", 0.0)},
          Vector3{
              doubleValue(fields, map, "tx", 0.0),
              doubleValue(fields, map, "ty", 0.0),
              doubleValue(fields, map, "tz", 0.0)});
      object.group_mask.group =
          parseCollisionMask(valueOr(fields, map, "group", "1"));
      object.group_mask.mask =
          parseCollisionMask(valueOr(fields, map, "mask", "0xffffffff"));
      object.type = worldObjectTypeFromString(valueOr(fields, map, "type", "dynamic"));
      object.enabled = boolValue(fields, map, "enabled", true);

      if (!object.model_path.empty()) {
        object.model = SDFBinReader::read(object.model_path);
      }
      if (options.require_models && !hasQueryableModel(object)) {
        result.error_message =
            "line " + std::to_string(line_number) +
            ": failed to load queryable SDF model";
        return result;
      }

      if (!object.samples_path.empty()) {
        CollisionSampleSetReadResult samples =
            CollisionSampleSetIO::readCSV(object.samples_path);
        if (!samples.success) {
          result.error_message =
              "line " + std::to_string(line_number) +
              ": failed to read sample CSV: " + samples.error_message;
          return result;
        }
        object.samples = std::move(samples.sample_set);
        object.has_samples = true;
        result.warnings.insert(
            result.warnings.end(),
            samples.warnings.begin(),
            samples.warnings.end());
      } else if (!options.allow_missing_samples) {
        result.error_message =
            "line " + std::to_string(line_number) + ": missing samples_path";
        return result;
      }

      result.world.addObject(std::move(object));
    } catch (const std::exception& exc) {
      result.error_message =
          "line " + std::to_string(line_number) + ": " + exc.what();
      return result;
    }
  }

  result.success = true;
  return result;
}

bool WorldSceneIO::writeCSV(
    const std::filesystem::path& path,
    const CollisionWorld& world,
    std::string* error_message) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream file(path);
  if (!file) {
    if (error_message) {
      *error_message = "failed to open world scene CSV: " + path.string();
    }
    return false;
  }

  file << "object_id,name,model_path,samples_path,tx,ty,tz,qw,qx,qy,qz,"
          "group,mask,type,enabled\n";
  for (const WorldObject& object : world.objects()) {
    const WorldQuaternion& q = object.transform.quaternion();
    const Vector3& t = object.transform.translation();
    file << object.object_id << ","
         << csvField(object.name) << ","
         << csvField(pathToUtf8(object.model_path)) << ","
         << csvField(pathToUtf8(object.samples_path)) << ","
         << t.x << "," << t.y << "," << t.z << ","
         << q.w << "," << q.x << "," << q.y << "," << q.z << ","
         << collisionMaskToString(object.group_mask.group) << ","
         << collisionMaskToString(object.group_mask.mask) << ","
         << toString(object.type) << ","
         << (object.enabled ? "true" : "false") << "\n";
  }
  return true;
}

}  // namespace adasdf
