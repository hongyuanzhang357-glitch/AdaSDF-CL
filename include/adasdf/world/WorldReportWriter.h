#pragma once

#include <filesystem>
#include <string>

#include "adasdf/world/AABBBroadphase.h"
#include "adasdf/world/WorldSolverContacts.h"
#include "adasdf/world/WorldSparseCollision.h"

namespace adasdf {

class WorldReportWriter {
 public:
  static std::string broadphaseToMarkdown(const AABBBroadphaseResult& result);
  static std::string broadphaseToJson(const AABBBroadphaseResult& result);
  static std::string sparseToMarkdown(const WorldSparseCollisionResult& result);
  static std::string sparseToJson(const WorldSparseCollisionResult& result);
  static std::string solverContactsToMarkdown(
      const WorldSolverContactResult& result);
  static std::string solverContactsToJson(
      const WorldSolverContactResult& result);

  static bool writeBroadphaseCSV(
      const std::filesystem::path& path,
      const AABBBroadphaseResult& result,
      std::string* error_message = nullptr);
  static bool writeSparseCSV(
      const std::filesystem::path& path,
      const WorldSparseCollisionResult& result,
      std::string* error_message = nullptr);
  static bool writeSolverContactsCSV(
      const std::filesystem::path& path,
      const WorldSolverContactResult& result,
      std::string* error_message = nullptr);
  static bool writeText(
      const std::filesystem::path& path,
      const std::string& text,
      std::string* error_message = nullptr);
};

}  // namespace adasdf
