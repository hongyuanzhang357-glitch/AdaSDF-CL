#include "adasdf/acceleration/BuildAccelerationReport.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace adasdf {
namespace {

std::string lower(std::string value) {
  std::transform(
      value.begin(),
      value.end(),
      value.begin(),
      [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return value;
}

std::string escaped(const std::string& value) {
  std::string out;
  for (const char ch : value) {
    if (ch == '"' || ch == '\\') {
      out.push_back('\\');
    }
    out.push_back(ch);
  }
  return out;
}

}  // namespace

const char* toString(SDFSamplingAcceleration acceleration) {
  switch (acceleration) {
    case SDFSamplingAcceleration::BruteForce:
      return "brute";
    case SDFSamplingAcceleration::BVH:
      return "bvh";
  }
  return "brute";
}

bool parseSDFSamplingAcceleration(
    const std::string& text,
    SDFSamplingAcceleration* acceleration) {
  const std::string value = lower(text);
  if (value == "brute" || value == "bruteforce" ||
      value == "brute-force") {
    if (acceleration != nullptr) {
      *acceleration = SDFSamplingAcceleration::BruteForce;
    }
    return true;
  }
  if (value == "bvh") {
    if (acceleration != nullptr) {
      *acceleration = SDFSamplingAcceleration::BVH;
    }
    return true;
  }
  return false;
}

std::string toMarkdown(const BuildAccelerationStats& stats) {
  std::ostringstream out;
  out << "## Build Acceleration\n\n";
  out << "- Acceleration: " << toString(stats.acceleration) << "\n";
  out << "- Used BVH: " << (stats.used_bvh ? "yes" : "no") << "\n";
  out << "- Threads requested: " << stats.threads_requested << "\n";
  out << "- Threads used: " << stats.threads_used << "\n";
  out << "- BVH nodes: " << stats.bvh_node_count << "\n";
  out << "- BVH leaves: " << stats.bvh_leaf_count << "\n";
  out << "- BVH triangles: " << stats.bvh_triangle_count << "\n";
  out << "- Samples: " << stats.sample_count << "\n";
  out << "- BVH build time ms: " << stats.bvh_build_time_ms << "\n";
  out << "- Sampling time ms: " << stats.sampling_time_ms << "\n";
  out << "- Brute reference time ms: " << stats.brute_reference_time_ms << "\n";
  out << "- Speedup vs brute reference: "
      << stats.speedup_vs_bruteforce << "\n";
  out << "- Nearest node visits: " << stats.nearest_node_visits << "\n";
  out << "- Nearest triangle tests: " << stats.nearest_triangle_tests << "\n";
  out << "- Ray node visits: " << stats.ray_node_visits << "\n";
  out << "- Ray triangle tests: " << stats.ray_triangle_tests << "\n";
  out << "- Ambiguous sign samples: " << stats.ambiguous_sign_count << "\n";
  out << "- Fallback samples: " << stats.fallback_count << "\n";
  if (!stats.warnings.empty()) {
    out << "\n### Warnings\n\n";
    for (const std::string& warning : stats.warnings) {
      out << "- " << warning << "\n";
    }
  }
  return out.str();
}

std::string toJson(const BuildAccelerationStats& stats) {
  std::ostringstream out;
  out << "{";
  out << "\"acceleration\":\"" << toString(stats.acceleration) << "\",";
  out << "\"used_bvh\":" << (stats.used_bvh ? "true" : "false") << ",";
  out << "\"threads_requested\":" << stats.threads_requested << ",";
  out << "\"threads_used\":" << stats.threads_used << ",";
  out << "\"bvh_node_count\":" << stats.bvh_node_count << ",";
  out << "\"bvh_leaf_count\":" << stats.bvh_leaf_count << ",";
  out << "\"bvh_triangle_count\":" << stats.bvh_triangle_count << ",";
  out << "\"sample_count\":" << stats.sample_count << ",";
  out << "\"nearest_triangle_tests\":" << stats.nearest_triangle_tests << ",";
  out << "\"nearest_node_visits\":" << stats.nearest_node_visits << ",";
  out << "\"ray_triangle_tests\":" << stats.ray_triangle_tests << ",";
  out << "\"ray_node_visits\":" << stats.ray_node_visits << ",";
  out << "\"ambiguous_sign_count\":" << stats.ambiguous_sign_count << ",";
  out << "\"fallback_count\":" << stats.fallback_count << ",";
  out << "\"bvh_build_time_ms\":" << stats.bvh_build_time_ms << ",";
  out << "\"sampling_time_ms\":" << stats.sampling_time_ms << ",";
  out << "\"brute_reference_time_ms\":" << stats.brute_reference_time_ms
      << ",";
  out << "\"speedup_vs_bruteforce\":" << stats.speedup_vs_bruteforce << ",";
  out << "\"warnings\":[";
  for (std::size_t i = 0; i < stats.warnings.size(); ++i) {
    if (i > 0) {
      out << ",";
    }
    out << "\"" << escaped(stats.warnings[i]) << "\"";
  }
  out << "]}";
  return out.str();
}

}  // namespace adasdf
