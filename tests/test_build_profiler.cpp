#include <adasdf/adasdf.h>

#include <iostream>

int main() {
  adasdf::BuildProfiler profiler(true);
  profiler.setToolName("unit");
  profiler.setInputPath("in.stl");
  profiler.setOutputPath("out.sdfbin");
  profiler.timings.load_mesh_time_ms = 1.0;
  profiler.counters.num_triangles = 12;
  profiler.finishCompleted();
  const std::string json = adasdf::BuildProfileJsonWriter::toJson(profiler);
  if (json.find("\"schema_id\": \"adasdf.build_profile.v1\"") ==
          std::string::npos ||
      json.find("\"profile_status\": \"completed\"") == std::string::npos ||
      json.find("\"num_triangles\":12") == std::string::npos) {
    std::cerr << "build profile JSON missing expected fields\n";
    return 1;
  }
  profiler.finishTimeout();
  const std::string timeout = adasdf::BuildProfileJsonWriter::toJson(profiler);
  if (timeout.find("ADASDF_TIMEOUT") == std::string::npos) {
    std::cerr << "timeout profile missing status code\n";
    return 1;
  }
  return 0;
}
