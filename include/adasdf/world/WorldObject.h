#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "adasdf/geometry/SDFModel.h"
#include "adasdf/sparse/CollisionSampleSet.h"
#include "adasdf/world/CollisionGroupMask.h"
#include "adasdf/world/WorldTransform.h"

namespace adasdf {

enum class WorldObjectType {
  Static,
  Dynamic
};

const char* toString(WorldObjectType type);
WorldObjectType worldObjectTypeFromString(const std::string& text);

struct WorldObject {
  int object_id = -1;
  std::string name;

  std::shared_ptr<SDFModel> model;
  CollisionSampleSet samples;
  bool has_samples = false;

  WorldTransform transform;
  CollisionGroupMask group_mask;
  WorldObjectType type = WorldObjectType::Dynamic;
  bool enabled = true;

  AABB local_aabb;
  AABB world_aabb;

  std::filesystem::path model_path;
  std::filesystem::path samples_path;
};

bool isStatic(const WorldObject& object);
bool hasQueryableModel(const WorldObject& object);
std::string worldObjectDisplayName(const WorldObject& object);

}  // namespace adasdf
