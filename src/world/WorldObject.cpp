#include "adasdf/world/WorldObject.h"

#include <stdexcept>

namespace adasdf {

const char* toString(WorldObjectType type) {
  switch (type) {
    case WorldObjectType::Static:
      return "static";
    case WorldObjectType::Dynamic:
      return "dynamic";
  }
  return "unknown";
}

WorldObjectType worldObjectTypeFromString(const std::string& text) {
  if (text == "static" || text == "Static" || text == "STATIC") {
    return WorldObjectType::Static;
  }
  if (text == "dynamic" || text == "Dynamic" || text == "DYNAMIC" ||
      text.empty()) {
    return WorldObjectType::Dynamic;
  }
  throw std::runtime_error("world object type must be static or dynamic");
}

bool isStatic(const WorldObject& object) {
  return object.type == WorldObjectType::Static;
}

bool hasQueryableModel(const WorldObject& object) {
  return object.model && object.model->isValid() &&
      object.model->queryBackendAvailable();
}

std::string worldObjectDisplayName(const WorldObject& object) {
  if (!object.name.empty()) {
    return object.name;
  }
  return "object_" + std::to_string(object.object_id);
}

}  // namespace adasdf
