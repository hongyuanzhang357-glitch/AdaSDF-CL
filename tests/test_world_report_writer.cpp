#include <adasdf/adasdf.h>

#include <filesystem>
#include <iostream>

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  adasdf::CollisionWorld world;
  adasdf::WorldObject a;
  a.object_id = 0;
  a.name = "a";
  a.model = adasdf::AnalyticSDFModel::createBox();
  world.addObject(a);
  adasdf::WorldObject b = a;
  b.object_id = 1;
  b.name = "b";
  world.addObject(b);
  const adasdf::AABBBroadphaseResult broadphase =
      adasdf::AABBBroadphase::compute(world);
  const std::string markdown =
      adasdf::WorldReportWriter::broadphaseToMarkdown(broadphase);
  if (markdown.find("CollisionWorld Broadphase") == std::string::npos) {
    std::cerr << "world broadphase markdown report failed\n";
    return 1;
  }
  std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  std::string error;
  if (!adasdf::WorldReportWriter::writeBroadphaseCSV(
          temp / "world_broadphase_report.csv",
          broadphase,
          &error)) {
    std::cerr << "world broadphase CSV report failed: " << error << "\n";
    return 1;
  }
  std::cout << "world report writer passed\n";
  return 0;
}
