#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "adasdf/world/CollisionWorld.h"

namespace adasdf {

struct WorldSceneReadOptions {
  bool require_models = true;
  bool allow_missing_samples = true;
};

struct WorldSceneReadResult {
  bool success = false;
  std::string error_message;
  CollisionWorld world;
  std::vector<std::string> warnings;
};

class WorldSceneIO {
 public:
  static WorldSceneReadResult readCSV(
      const std::filesystem::path& path,
      const WorldSceneReadOptions& options = WorldSceneReadOptions{});

  static bool writeCSV(
      const std::filesystem::path& path,
      const CollisionWorld& world,
      std::string* error_message = nullptr);
};

}  // namespace adasdf
