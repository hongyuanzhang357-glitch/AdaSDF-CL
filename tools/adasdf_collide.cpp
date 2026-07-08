#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

#include "ModelJsonHelpers.h"

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_collide object_a.sdfbin object_b.sdfbin [options]\n"
      << "Options:\n"
      << "  --offset dx dy dz       World-space translation applied to object B\n"
      << "  --max-contacts value    Maximum contacts to report, default 32\n"
      << "  --tolerance value       Contact tolerance, default 1e-4\n"
      << "  --grid-resolution value Candidate grid resolution, default 8\n"
      << "  --backend cpu|cuda      Query backend, default cpu\n"
      << "  --expansion none|global|block\n"
      << "  --blocks all|0,1,2      Block selection for block expansion\n"
      << "  --no-contact            Only report collision state and distance\n"
      << "  --json                  Emit stable adasdf.collide.v1 JSON\n";
}

bool hasValue(int index, int argc, int count = 1) {
  return index + count < argc;
}

void printVec(const char* label, const adasdf::Vector3& v) {
  std::cout << label << v.x << " " << v.y << " " << v.z << "\n";
}

void printAABB(const char* label, const adasdf::AABB& aabb) {
  std::cout << label;
  if (!aabb.valid) {
    std::cout << "invalid\n";
    return;
  }
  std::cout << "min=(" << aabb.min << ") max=(" << aabb.max << ")\n";
}

std::vector<std::string> splitList(const std::string& text) {
  std::vector<std::string> values;
  std::string current;
  for (const char ch : text) {
    if (ch == ',') {
      if (!current.empty()) {
        values.push_back(current);
      }
      current.clear();
    } else {
      current.push_back(ch);
    }
  }
  if (!current.empty()) {
    values.push_back(current);
  }
  return values;
}

adasdf::QueryBackend parseQueryBackend(const std::string& text) {
  if (text == "cpu") {
    return adasdf::QueryBackend::CPU;
  }
  if (text == "cuda") {
    return adasdf::QueryBackend::CUDA;
  }
  if (text == "auto") {
    return adasdf::QueryBackend::Auto;
  }
  throw std::runtime_error("backend must be cpu, cuda, or auto");
}

adasdf::BackendType toBackendType(adasdf::QueryBackend backend) {
  switch (backend) {
    case adasdf::QueryBackend::CPU:
      return adasdf::BackendType::CPU;
    case adasdf::QueryBackend::CUDA:
      return adasdf::BackendType::CUDA;
    case adasdf::QueryBackend::Auto:
      return adasdf::BackendType::Auto;
  }
  return adasdf::BackendType::CPU;
}

adasdf::QueryExpansionMode parseExpansion(const std::string& text) {
  if (text == "none") {
    return adasdf::QueryExpansionMode::None;
  }
  if (text == "global") {
    return adasdf::QueryExpansionMode::Global;
  }
  if (text == "block") {
    return adasdf::QueryExpansionMode::Block;
  }
  if (text == "auto") {
    return adasdf::QueryExpansionMode::Auto;
  }
  throw std::runtime_error("expansion must be none, global, block, or auto");
}

adasdf::BlockSelection parseBlocks(const std::string& text) {
  if (text.empty() || text == "all") {
    return adasdf::BlockSelection::all();
  }
  std::vector<int> ids;
  for (const std::string& item : splitList(text)) {
    ids.push_back(std::stoi(item));
  }
  return adasdf::BlockSelection::selected(std::move(ids));
}

std::string contactsJson(const adasdf::CollisionResult& result) {
  std::ostringstream out;
  out << "[";
  const auto& contacts = result.contacts();
  for (std::size_t i = 0; i < contacts.size(); ++i) {
    const adasdf::Contact& contact = contacts[i];
    if (i != 0) {
      out << ",";
    }
    out << "{\"point\":" << adasdf::JsonContractWriter::vec3(contact.point)
        << ",\"normal\":" << adasdf::JsonContractWriter::vec3(contact.normal)
        << ",\"phi\":"
        << adasdf::JsonContractWriter::number(contact.signed_distance)
        << ",\"penetration_depth\":"
        << adasdf::JsonContractWriter::number(contact.penetration_depth)
        << ",\"object_id_a\":" << contact.object_id_a
        << ",\"object_id_b\":" << contact.object_id_b
        << ",\"block_id\":-1,\"source\":\"adasdf_collide\"}";
  }
  out << "]";
  return out.str();
}

}  // namespace

int main(int argc, char** argv) {
  using namespace adasdf;

  if (argc == 1) {
    usage();
    return 0;
  }
  if (argc < 3) {
    usage();
    return 2;
  }

  const std::filesystem::path path_a = argv[1];
  const std::filesystem::path path_b = argv[2];
  Vector3 offset{0.0, 0.0, 0.0};

  CollisionRequest request;
  request.enable_contact = true;
  request.max_contacts = 32;
  request.contact_tolerance = 1.0e-4;
  request.enable_gradient = true;
  request.backend = BackendType::CPU;
  request.query_mode = QueryMode::Balanced;
  bool expansion_set = false;
  bool json = false;

  for (int i = 3; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--offset" && hasValue(i, argc, 3)) {
      offset = {std::stod(argv[++i]), std::stod(argv[++i]), std::stod(argv[++i])};
    } else if (arg == "--max-contacts" && hasValue(i, argc)) {
      request.max_contacts = std::stoi(argv[++i]);
    } else if (arg == "--tolerance" && hasValue(i, argc)) {
      request.contact_tolerance = std::stod(argv[++i]);
    } else if (arg == "--grid-resolution" && hasValue(i, argc)) {
      request.candidate_grid_resolution = std::stoi(argv[++i]);
    } else if (arg == "--backend" && hasValue(i, argc)) {
      request.query_mode_config.backend = parseQueryBackend(argv[++i]);
      request.backend = toBackendType(request.query_mode_config.backend);
    } else if (arg == "--expansion" && hasValue(i, argc)) {
      request.query_mode_config.expansion = parseExpansion(argv[++i]);
      expansion_set = true;
    } else if (arg == "--blocks" && hasValue(i, argc)) {
      request.query_mode_config.block_selection = parseBlocks(argv[++i]);
    } else if (arg == "--no-contact") {
      request.enable_contact = false;
      request.max_contacts = 0;
    } else if (arg == "--json") {
      json = true;
    } else if (arg == "--help" || arg == "-h") {
      usage();
      return 0;
    } else {
      std::cerr << "Unknown or incomplete option: " << arg << "\n";
      usage();
      return 2;
    }
  }
  if (!expansion_set && request.query_mode_config.backend == QueryBackend::CUDA) {
    request.query_mode_config.expansion = QueryExpansionMode::Global;
  }

  if (!std::filesystem::exists(path_a)) {
    std::cerr << "adasdf_collide: file does not exist: " << path_a.string() << "\n";
    return 2;
  }
  if (!std::filesystem::exists(path_b)) {
    std::cerr << "adasdf_collide: file does not exist: " << path_b.string() << "\n";
    return 2;
  }

  try {
    const auto model_a = SDFBinReader::read(path_a);
    const auto model_b = SDFBinReader::read(path_b);

    CollisionObject object_a(model_a);
    CollisionObject object_b(model_b);
    object_a.setTransform(Transform::Identity());
    object_b.setTransform(Transform::fromTranslation(offset));

    CollisionResult result;
    const bool hit = collide(object_a, object_b, request, result);

    if (json) {
      BackendJsonContract contract = adasdf_tools::makeBaseContract(
          SchemaIds::Collide,
          "adasdf_collide");
      contract.status_code = hit ? ErrorCode::OK : ErrorCode::NO_COLLISION;
      contract.payload_fields.push_back(
          {"mode", JsonContractWriter::quote("pair")});
      contract.payload_fields.push_back(
          {"collided", JsonContractWriter::boolean(hit)});
      contract.payload_fields.push_back(
          {"candidate_count",
           JsonContractWriter::integer(result.numCandidatePoints())});
      contract.payload_fields.push_back(
          {"contact_count", JsonContractWriter::integer(result.contacts().size())});
      contract.payload_fields.push_back(
          {"contacts", contactsJson(result)});
      contract.payload_fields.push_back(
          {"minimum_distance",
           JsonContractWriter::number(result.minimumDistance())});
      contract.payload_fields.push_back(
          {"backend", JsonContractWriter::quote(result.backendInfo())});
      contract.payload_fields.push_back(
          {"requested_query_backend",
           JsonContractWriter::quote(toString(request.query_mode_config.backend))});
      contract.payload_fields.push_back(
          {"requested_expansion",
           JsonContractWriter::quote(toString(request.query_mode_config.expansion))});
      std::cout << JsonContractWriter::writeObject(contract);
      return 0;
    }

    std::cout << "AdaSDF-CL collision query\n";
    std::cout << "Model A: " << path_a.string() << "\n";
    std::cout << "Model B: " << path_b.string() << "\n";
    printVec("Offset B: ", offset);
    printAABB("AABB A: ", object_a.getAABB());
    printAABB("AABB B: ", object_b.getAABB());
    std::cout << "Colliding: " << (hit ? "true" : "false") << "\n";
    std::cout << "Minimum distance: " << result.minimumDistance() << "\n";
    std::cout << "Backend: " << result.backendInfo() << "\n";
    std::cout << "Requested query backend: "
              << toString(request.query_mode_config.backend) << "\n";
    std::cout << "Requested expansion: "
              << toString(request.query_mode_config.expansion) << "\n";
    std::cout << "Requested blocks: "
              << blockSelectionString(request.query_mode_config.block_selection) << "\n";
    std::cout << "Method: " << result.methodInfo() << "\n";
    std::cout << "Candidate points: " << result.numCandidatePoints() << "\n";
    std::cout << "Raw contacts: " << result.numRawContacts() << "\n";
    std::cout << "Reduced contacts: " << result.numReducedContacts() << "\n";
    std::cout << "Requested max contacts: " << request.max_contacts << "\n";
    std::cout << "Returned contacts: " << result.contacts().size() << "\n";
    std::cout << "Contact count: " << result.contacts().size() << "\n";
    if (!result.contacts().empty()) {
      const Contact& contact = result.contacts().front();
      printVec("Contact[0].point: ", contact.point);
      printVec("Contact[0].normal: ", contact.normal);
      std::cout << "Contact[0].penetration_depth: "
                << contact.penetration_depth << "\n";
      std::cout << "Contact[0].signed_distance: "
                << contact.signed_distance << "\n";
    }
    std::cout << "Number of SDF queries: " << result.numSDFQueries() << "\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_collide failed: " << exc.what() << "\n";
    return 1;
  }
}
