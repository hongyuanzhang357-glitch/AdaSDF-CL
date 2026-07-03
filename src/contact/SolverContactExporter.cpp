#include "adasdf/contact/SolverContactExporter.h"

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

bool prepareFile(
    const std::string& path,
    std::ofstream& file,
    std::string* error_message) {
  const std::filesystem::path out_path(path);
  if (!out_path.parent_path().empty()) {
    std::filesystem::create_directories(out_path.parent_path());
  }
  file.open(out_path);
  if (!file) {
    if (error_message) {
      *error_message = "failed to open output file: " + path;
    }
    return false;
  }
  file << std::setprecision(10);
  return true;
}

double maxPenetration(const SolverContactSet& contacts) {
  double value = 0.0;
  for (const SolverContact& contact : contacts.contacts) {
    value = std::max(value, contact.penetration_depth);
  }
  return value;
}

}  // namespace

std::string SolverContactExporter::toMarkdown(const SolverContactSet& contacts) {
  std::ostringstream out;
  out << "# Solver-Ready Contact Candidates\n\n";
  out << "- Contact count: " << contacts.size() << "\n";
  out << "- Max penetration: " << maxPenetration(contacts) << "\n\n";
  out << "| contact_id | sample_id | patch_id | penetration_depth | stable_key | label |\n";
  out << "| --- | --- | --- | --- | --- | --- |\n";
  for (const SolverContact& contact : contacts.contacts) {
    out << "| " << contact.contact_id
        << " | " << contact.sample_id
        << " | " << contact.patch_id
        << " | " << contact.penetration_depth
        << " | " << contact.stable_key
        << " | " << contact.label << " |\n";
  }
  out << "\nLimitations: this exports solver-ready candidates, not solver "
         "constraints. No impulses or friction forces are computed.\n";
  return out.str();
}

std::string SolverContactExporter::toJson(const SolverContactSet& contacts) {
  std::ostringstream out;
  out << "{\n";
  out << "  \"contact_count\": " << contacts.size() << ",\n";
  out << "  \"max_penetration\": " << maxPenetration(contacts) << ",\n";
  out << "  \"limitations\": \"not a solver; no impulses or friction computed\",\n";
  out << "  \"contacts\": [\n";
  for (std::size_t i = 0; i < contacts.contacts.size(); ++i) {
    const SolverContact& contact = contacts.contacts[i];
    out << "    {"
        << "\"contact_id\": " << contact.contact_id
        << ", \"sample_id\": " << contact.sample_id
        << ", \"patch_id\": " << contact.patch_id
        << ", \"x\": " << contact.point.x
        << ", \"y\": " << contact.point.y
        << ", \"z\": " << contact.point.z
        << ", \"nx\": " << contact.normal.x
        << ", \"ny\": " << contact.normal.y
        << ", \"nz\": " << contact.normal.z
        << ", \"penetration_depth\": " << contact.penetration_depth
        << ", \"phi\": " << contact.phi
        << ", \"effective_phi\": " << contact.effective_phi
        << ", \"object_id\": " << contact.object_id
        << ", \"link_id\": " << contact.link_id
        << ", \"group_id\": " << contact.group_id
        << ", \"stable_key\": " << jsonField(contact.stable_key)
        << ", \"label\": " << jsonField(contact.label) << "}";
    if (i + 1 < contacts.contacts.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ]\n";
  out << "}\n";
  return out.str();
}

bool SolverContactExporter::writeCSV(
    const std::string& path,
    const SolverContactSet& contacts,
    std::string* error_message) {
  std::ofstream file;
  if (!prepareFile(path, file, error_message)) {
    return false;
  }
  file << "contact_id,sample_id,patch_id,x,y,z,nx,ny,nz,"
          "penetration_depth,phi,effective_phi,object_id,link_id,group_id,"
          "stable_key,label\n";
  for (const SolverContact& contact : contacts.contacts) {
    file << contact.contact_id << ","
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
  return true;
}

bool SolverContactExporter::writeJson(
    const std::string& path,
    const SolverContactSet& contacts,
    std::string* error_message) {
  std::ofstream file;
  if (!prepareFile(path, file, error_message)) {
    return false;
  }
  file << toJson(contacts);
  return true;
}

}  // namespace adasdf
