#include <adasdf/adasdf.h>

#include <algorithm>
#include <cctype>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_stabilize_contacts candidates.csv "
         "[--max-contacts N] [--max-contacts-per-link N] "
         "[--max-contacts-per-patch N] [--patch-radius value] "
         "[--normal-cos value] [--min-penetration value] [--no-cluster] "
         "[--no-normal-consistency] [--no-duplicate-removal] "
         "[--out solver_contacts.csv] [--json solver_contacts.json] "
         "[--report stabilized.md]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

std::string trim(const std::string& text) {
  std::size_t first = 0;
  while (first < text.size() &&
         std::isspace(static_cast<unsigned char>(text[first]))) {
    ++first;
  }
  std::size_t last = text.size();
  while (last > first &&
         std::isspace(static_cast<unsigned char>(text[last - 1]))) {
    --last;
  }
  return text.substr(first, last - first);
}

std::string lower(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return text;
}

std::vector<std::string> splitCSV(const std::string& line) {
  std::vector<std::string> fields;
  std::string current;
  bool quoted = false;
  for (std::size_t i = 0; i < line.size(); ++i) {
    const char ch = line[i];
    if (quoted) {
      if (ch == '"' && i + 1 < line.size() && line[i + 1] == '"') {
        current.push_back('"');
        ++i;
      } else if (ch == '"') {
        quoted = false;
      } else {
        current.push_back(ch);
      }
    } else if (ch == '"') {
      quoted = true;
    } else if (ch == ',') {
      fields.push_back(trim(current));
      current.clear();
    } else {
      current.push_back(ch);
    }
  }
  fields.push_back(trim(current));
  return fields;
}

std::size_t column(
    const std::unordered_map<std::string, std::size_t>& columns,
    const std::string& name,
    std::size_t fallback) {
  const auto iter = columns.find(name);
  return iter == columns.end() ? fallback : iter->second;
}

double parseDouble(
    const std::vector<std::string>& fields,
    std::size_t index,
    double default_value) {
  if (index >= fields.size() || trim(fields[index]).empty()) {
    return default_value;
  }
  return std::stod(fields[index]);
}

int parseInt(
    const std::vector<std::string>& fields,
    std::size_t index,
    int default_value) {
  if (index >= fields.size() || trim(fields[index]).empty()) {
    return default_value;
  }
  return std::stoi(fields[index]);
}

struct CandidateReadResult {
  bool success = false;
  std::string error_message;
  std::vector<adasdf::ContactCandidate> candidates;
};

CandidateReadResult readCandidatesCSV(const std::filesystem::path& path) {
  CandidateReadResult result;
  std::ifstream file(path);
  if (!file) {
    result.error_message = "failed to open candidate CSV: " + path.string();
    return result;
  }

  std::unordered_map<std::string, std::size_t> columns;
  bool header_seen = false;
  bool first = true;
  std::string line;
  int line_number = 0;
  while (std::getline(file, line)) {
    ++line_number;
    const std::string trimmed = trim(line);
    if (trimmed.empty() || trimmed.front() == '#') {
      continue;
    }
    const std::vector<std::string> fields = splitCSV(line);
    if (first) {
      first = false;
      if (!fields.empty() && lower(fields[0]) == "rank") {
        header_seen = true;
        for (std::size_t i = 0; i < fields.size(); ++i) {
          columns[lower(trim(fields[i]))] = i;
        }
        continue;
      }
    }
    try {
      adasdf::ContactCandidate candidate;
      candidate.rank = parseInt(fields, header_seen ? column(columns, "rank", 0) : 0, -1);
      candidate.sample_id =
          parseInt(fields, header_seen ? column(columns, "sample_id", 1) : 1, -1);
      candidate.point = {
          parseDouble(fields, header_seen ? column(columns, "x", 2) : 2, 0.0),
          parseDouble(fields, header_seen ? column(columns, "y", 3) : 3, 0.0),
          parseDouble(fields, header_seen ? column(columns, "z", 4) : 4, 0.0)};
      candidate.radius =
          parseDouble(fields, header_seen ? column(columns, "radius", 5) : 5, 0.0);
      candidate.phi =
          parseDouble(fields, header_seen ? column(columns, "phi", 6) : 6, 0.0);
      candidate.effective_phi = parseDouble(
          fields,
          header_seen ? column(columns, "effective_phi", 7) : 7,
          candidate.phi);
      candidate.penetration_depth = parseDouble(
          fields,
          header_seen ? column(columns, "penetration_depth", 8) : 8,
          std::max(0.0, -candidate.effective_phi));
      candidate.normal = {
          parseDouble(fields, header_seen ? column(columns, "normal_x", 9) : 9, 0.0),
          parseDouble(fields, header_seen ? column(columns, "normal_y", 10) : 10, 0.0),
          parseDouble(fields, header_seen ? column(columns, "normal_z", 11) : 11, 0.0)};
      candidate.has_normal = candidate.normal.allFinite() &&
                             (candidate.normal.x != 0.0 ||
                              candidate.normal.y != 0.0 ||
                              candidate.normal.z != 0.0);
      candidate.object_id = parseInt(
          fields,
          header_seen ? column(columns, "object_id", 12) : 12,
          0);
      candidate.link_id =
          parseInt(fields, header_seen ? column(columns, "link_id", 13) : 13, 0);
      candidate.group_id = parseInt(
          fields,
          header_seen ? column(columns, "group_id", 14) : 14,
          0);
      const std::size_t label_col = header_seen ? column(columns, "label", 15) : 15;
      if (label_col < fields.size()) {
        candidate.label = fields[label_col];
      }
      result.candidates.push_back(candidate);
    } catch (const std::exception& exc) {
      result.error_message =
          "line " + std::to_string(line_number) + ": " + exc.what();
      return result;
    }
  }
  result.success = true;
  return result;
}

bool writeText(const std::filesystem::path& path, const std::string& text) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  std::ofstream file(path);
  if (!file) {
    return false;
  }
  file << text;
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1 || (argc > 1 && std::string(argv[1]) == "--help")) {
      usage();
      return 0;
    }
    if (argc < 2) {
      usage();
      return 1;
    }

    const std::filesystem::path candidates_path = argv[1];
    adasdf::ContactStabilizationOptions options;
    std::filesystem::path out_path;
    std::filesystem::path json_path;
    std::filesystem::path report_path;

    for (int i = 2; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--max-contacts" && hasValue(i, argc)) {
        options.budget.max_contacts_total = std::stoi(argv[++i]);
      } else if (arg == "--max-contacts-per-link" && hasValue(i, argc)) {
        options.budget.max_contacts_per_link = std::stoi(argv[++i]);
        options.budget.enforce_link_budget = true;
      } else if (arg == "--max-contacts-per-patch" && hasValue(i, argc)) {
        options.budget.max_contacts_per_patch = std::stoi(argv[++i]);
      } else if (arg == "--patch-radius" && hasValue(i, argc)) {
        options.patch_options.spatial_radius = std::stod(argv[++i]);
      } else if (arg == "--normal-cos" && hasValue(i, argc)) {
        options.patch_options.normal_cosine_threshold = std::stod(argv[++i]);
      } else if (arg == "--min-penetration" && hasValue(i, argc)) {
        options.min_penetration_depth = std::stod(argv[++i]);
      } else if (arg == "--no-cluster") {
        options.cluster_candidates = false;
      } else if (arg == "--no-normal-consistency") {
        options.enforce_normal_consistency = false;
        options.patch_options.require_normal_consistency = false;
      } else if (arg == "--no-duplicate-removal") {
        options.remove_duplicate_samples = false;
      } else if (arg == "--out" && hasValue(i, argc)) {
        out_path = argv[++i];
      } else if (arg == "--json" && hasValue(i, argc)) {
        json_path = argv[++i];
      } else if (arg == "--report" && hasValue(i, argc)) {
        report_path = argv[++i];
      } else if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else {
        std::cerr << "Unknown or incomplete option: " << arg << "\n";
        usage();
        return 1;
      }
    }

    const CandidateReadResult read = readCandidatesCSV(candidates_path);
    if (!read.success) {
      std::cerr << "adasdf_stabilize_contacts: " << read.error_message << "\n";
      return 1;
    }
    const adasdf::ContactStabilizationResult stabilized =
        adasdf::ContactStabilizer::stabilize(read.candidates, options);
    if (!stabilized.success) {
      std::cerr << "adasdf_stabilize_contacts: "
                << stabilized.error_message << "\n";
      return 2;
    }
    const adasdf::SolverContactSet contacts =
        adasdf::SolverContactBuilder::fromCandidates(
            stabilized.stabilized_candidates,
            stabilized.patches);

    std::cout << "AdaSDF-CL stabilize contacts\n";
    std::cout << "Input candidates: " << read.candidates.size() << "\n";
    std::cout << "Patches: " << stabilized.stats.patch_count << "\n";
    std::cout << "Output solver contacts: " << contacts.size() << "\n";
    std::cout << "Max contacts: " << options.budget.max_contacts_total << "\n";
    std::cout << "Warnings: " << stabilized.stats.warnings.size() << "\n";
    std::cout << "This exports solver-ready candidates, not solver constraints.\n";
    std::cout << "No impulses or friction forces are computed.\n";
    std::cout << "Status: ok\n";

    std::string error;
    if (!out_path.empty() &&
        !adasdf::SolverContactExporter::writeCSV(
            out_path.string(), contacts, &error)) {
      std::cerr << "adasdf_stabilize_contacts: failed to write CSV: "
                << error << "\n";
      return 3;
    }
    if (!json_path.empty() &&
        !adasdf::SolverContactExporter::writeJson(
            json_path.string(), contacts, &error)) {
      std::cerr << "adasdf_stabilize_contacts: failed to write JSON: "
                << error << "\n";
      return 3;
    }
    if (!report_path.empty() &&
        !writeText(
            report_path,
            adasdf::ContactStabilizationReportWriter::toMarkdown(stabilized))) {
      std::cerr << "adasdf_stabilize_contacts: failed to write report\n";
      return 3;
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_stabilize_contacts failed: " << exc.what() << "\n";
    return 2;
  }
}
