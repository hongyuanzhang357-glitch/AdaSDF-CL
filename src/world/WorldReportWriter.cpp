#include "adasdf/world/WorldReportWriter.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace adasdf {
namespace {

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

std::string jsonField(const std::string& value) {
  std::string escaped = "\"";
  for (const char ch : value) {
    switch (ch) {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        escaped.push_back(ch);
        break;
    }
  }
  escaped += "\"";
  return escaped;
}

double maxPenetration(const SolverContactSet& contacts) {
  double value = 0.0;
  for (const SolverContact& contact : contacts.contacts) {
    value = std::max(value, contact.penetration_depth);
  }
  return value;
}

}  // namespace

std::string WorldReportWriter::broadphaseToMarkdown(
    const AABBBroadphaseResult& result) {
  std::ostringstream out;
  out << "# CollisionWorld Broadphase Report\n\n";
  out << "- Object count: " << result.stats.object_count << "\n";
  out << "- Tested pairs: " << result.stats.tested_pair_count << "\n";
  out << "- Overlap pairs: " << result.stats.overlap_pair_count << "\n";
  out << "- Group/mask skipped: " << result.stats.group_mask_pair_skipped << "\n";
  out << "- Static-static skipped: "
      << result.stats.static_static_pair_skipped << "\n";
  out << "- AABB rejected: " << result.stats.aabb_rejected_count << "\n\n";
  out << "| pair_id | object_a | object_b | name_a | name_b |\n";
  out << "| --- | --- | --- | --- | --- |\n";
  for (const WorldPair& pair : result.pairs) {
    out << "| " << pair.pair_id << " | " << pair.object_a << " | "
        << pair.object_b << " | " << pair.name_a << " | " << pair.name_b
        << " |\n";
  }
  out << "\nBroadphase uses deterministic AABB O(N^2) filtering.\n";
  return out.str();
}

std::string WorldReportWriter::broadphaseToJson(
    const AABBBroadphaseResult& result) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"object_count\": " << result.stats.object_count << ",\n";
  out << "  \"tested_pair_count\": " << result.stats.tested_pair_count << ",\n";
  out << "  \"overlap_pair_count\": " << result.stats.overlap_pair_count << ",\n";
  out << "  \"pairs\": [\n";
  for (std::size_t i = 0; i < result.pairs.size(); ++i) {
    const WorldPair& pair = result.pairs[i];
    out << "    {\"pair_id\": " << pair.pair_id
        << ", \"object_a\": " << pair.object_a
        << ", \"object_b\": " << pair.object_b
        << ", \"name_a\": " << jsonField(pair.name_a)
        << ", \"name_b\": " << jsonField(pair.name_b) << "}";
    if (i + 1 < result.pairs.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ]\n";
  out << "}\n";
  return out.str();
}

std::string WorldReportWriter::sparseToMarkdown(
    const WorldSparseCollisionResult& result) {
  std::ostringstream out;
  out << "# CollisionWorld Sparse Collision Report\n\n";
  out << "- Colliding: " << (result.colliding ? "true" : "false") << "\n";
  out << "- Broadphase pairs: " << result.stats.broadphase_pair_count << "\n";
  out << "- Queried pairs: " << result.stats.queried_pair_count << "\n";
  out << "- Queried samples: " << result.stats.queried_sample_count << "\n";
  out << "- Violations: " << result.stats.violation_count << "\n";
  out << "- Min effective phi: " << result.stats.min_effective_phi << "\n\n";
  out << "| pair_id | object_a | object_b | colliding | violations |\n";
  out << "| --- | --- | --- | --- | --- |\n";
  for (const WorldSparsePairResult& pair : result.pairs) {
    out << "| " << pair.pair_id << " | " << pair.name_a << " | "
        << pair.name_b << " | " << (pair.colliding ? "true" : "false")
        << " | " << pair.violation_count << " |\n";
  }
  out << "\nThis is sample-based SDF collision, not exact mesh-vs-mesh contact.\n";
  return out.str();
}

std::string WorldReportWriter::sparseToJson(
    const WorldSparseCollisionResult& result) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"colliding\": " << (result.colliding ? "true" : "false") << ",\n";
  out << "  \"broadphase_pair_count\": "
      << result.stats.broadphase_pair_count << ",\n";
  out << "  \"queried_pair_count\": " << result.stats.queried_pair_count << ",\n";
  out << "  \"violation_count\": " << result.stats.violation_count << ",\n";
  out << "  \"pairs\": [\n";
  for (std::size_t i = 0; i < result.pairs.size(); ++i) {
    const WorldSparsePairResult& pair = result.pairs[i];
    out << "    {\"pair_id\": " << pair.pair_id
        << ", \"object_a\": " << pair.object_a
        << ", \"object_b\": " << pair.object_b
        << ", \"colliding\": " << (pair.colliding ? "true" : "false")
        << ", \"violation_count\": " << pair.violation_count << "}";
    if (i + 1 < result.pairs.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ]\n";
  out << "}\n";
  return out.str();
}

std::string WorldReportWriter::solverContactsToMarkdown(
    const WorldSolverContactResult& result) {
  std::ostringstream out;
  out << "# CollisionWorld Solver-Ready Contacts\n\n";
  out << "- Solver contacts: " << result.contacts.size() << "\n";
  out << "- Raw candidates: " << result.stats.raw_candidate_count << "\n";
  out << "- Reduced candidates: " << result.stats.reduced_candidate_count << "\n";
  out << "- Patches: " << result.stats.patch_count << "\n";
  out << "- Max penetration: " << maxPenetration(result.contacts) << "\n\n";
  out << "| contact_id | object_id | link_id | pair_id | penetration_depth | stable_key |\n";
  out << "| --- | --- | --- | --- | --- | --- |\n";
  for (const SolverContact& contact : result.contacts.contacts) {
    out << "| " << contact.contact_id << " | " << contact.object_id << " | "
        << contact.link_id << " | " << contact.group_id << " | "
        << contact.penetration_depth << " | " << contact.stable_key << " |\n";
  }
  out << "\nThese are solver-ready candidates only. No impulses, friction forces, "
         "or solver constraints are computed.\n";
  return out.str();
}

std::string WorldReportWriter::solverContactsToJson(
    const WorldSolverContactResult& result) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"solver_contact_count\": " << result.contacts.size() << ",\n";
  out << "  \"raw_candidate_count\": "
      << result.stats.raw_candidate_count << ",\n";
  out << "  \"max_penetration\": " << maxPenetration(result.contacts) << ",\n";
  out << "  \"limitations\": \"solver-ready candidates only; no solver constraints\",\n";
  out << "  \"contacts\": [\n";
  for (std::size_t i = 0; i < result.contacts.contacts.size(); ++i) {
    const SolverContact& contact = result.contacts.contacts[i];
    out << "    {\"contact_id\": " << contact.contact_id
        << ", \"object_id\": " << contact.object_id
        << ", \"link_id\": " << contact.link_id
        << ", \"pair_id\": " << contact.group_id
        << ", \"penetration_depth\": " << contact.penetration_depth
        << ", \"stable_key\": " << jsonField(contact.stable_key)
        << ", \"label\": " << jsonField(contact.label) << "}";
    if (i + 1 < result.contacts.contacts.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ]\n";
  out << "}\n";
  return out.str();
}

bool WorldReportWriter::writeBroadphaseCSV(
    const std::filesystem::path& path,
    const AABBBroadphaseResult& result,
    std::string* error_message) {
  std::ostringstream out;
  out << std::setprecision(10);
  out << "pair_id,object_a,object_b,name_a,name_b,type_a,type_b,"
         "group_mask_allowed,aabb_overlap\n";
  for (const WorldPair& pair : result.pairs) {
    out << pair.pair_id << "," << pair.object_a << "," << pair.object_b << ","
        << csvField(pair.name_a) << "," << csvField(pair.name_b) << ","
        << toString(pair.type_a) << "," << toString(pair.type_b) << ","
        << (pair.group_mask_allowed ? "true" : "false") << ","
        << (pair.aabb_overlap ? "true" : "false") << "\n";
  }
  return writeText(path, out.str(), error_message);
}

bool WorldReportWriter::writeSparseCSV(
    const std::filesystem::path& path,
    const WorldSparseCollisionResult& result,
    std::string* error_message) {
  std::ostringstream out;
  out << std::setprecision(10);
  out << "pair_id,source_object_id,target_object_id,source_name,target_name,"
         "sample_id,x,y,z,radius,phi,effective_phi,colliding,label\n";
  for (const WorldSparsePairResult& pair : result.pairs) {
    for (const WorldSparseSampleResult& sample : pair.violations) {
      out << sample.pair_id << ","
          << sample.source_object_id << ","
          << sample.target_object_id << ","
          << csvField(sample.source_name) << ","
          << csvField(sample.target_name) << ","
          << sample.sample.sample_id << ","
          << sample.world_position.x << ","
          << sample.world_position.y << ","
          << sample.world_position.z << ","
          << sample.sample.radius << ","
          << sample.sample.phi << ","
          << sample.sample.effective_phi << ","
          << (sample.sample.colliding ? "true" : "false") << ","
          << csvField(sample.sample.label) << "\n";
    }
  }
  return writeText(path, out.str(), error_message);
}

bool WorldReportWriter::writeSolverContactsCSV(
    const std::filesystem::path& path,
    const WorldSolverContactResult& result,
    std::string* error_message) {
  std::ostringstream out;
  out << std::setprecision(10);
  out << "contact_id,sample_id,patch_id,x,y,z,nx,ny,nz,"
         "penetration_depth,phi,effective_phi,object_id,link_id,pair_id,"
         "stable_key,label\n";
  for (const SolverContact& contact : result.contacts.contacts) {
    out << contact.contact_id << ","
        << contact.sample_id << ","
        << contact.patch_id << ","
        << contact.point.x << ","
        << contact.point.y << ","
        << contact.point.z << ","
        << contact.normal.x << ","
        << contact.normal.y << ","
        << contact.normal.z << ","
        << contact.penetration_depth << ","
        << contact.phi << ","
        << contact.effective_phi << ","
        << contact.object_id << ","
        << contact.link_id << ","
        << contact.group_id << ","
        << csvField(contact.stable_key) << ","
        << csvField(contact.label) << "\n";
  }
  return writeText(path, out.str(), error_message);
}

bool WorldReportWriter::writeText(
    const std::filesystem::path& path,
    const std::string& text,
    std::string* error_message) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream file(path);
  if (!file) {
    if (error_message) {
      *error_message = "failed to open output file: " + path.string();
    }
    return false;
  }
  file << text;
  return true;
}

}  // namespace adasdf
