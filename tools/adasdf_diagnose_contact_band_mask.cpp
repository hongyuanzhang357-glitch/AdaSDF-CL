#include <adasdf/adasdf.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Options {
  std::filesystem::path stl;
  int max_level = 3;
  int min_level = 1;
  int block_resolution = 8;
  adasdf::AdaptiveLeafMode adaptive_leaf_mode = adasdf::AdaptiveLeafMode::Mixed;
  std::vector<double> contact_band_widths = {0.002, 0.004, 0.008};
  std::vector<int> halo_exact_layers = {1, 2, 3};
  std::vector<double> marker_safety_factors = {1.5, 2.0, 3.0};
  adasdf::ContactBandMarkerMode marker_mode =
      adasdf::ContactBandMarkerMode::DistanceAware;
  double marker_cell_size_factor = 0.75;
  bool local_halo_only = false;
  std::filesystem::path json;
  std::filesystem::path csv;
  std::filesystem::path report;
};

struct Row {
  std::string case_id;
  double contact_band_width = 0.0;
  int halo_exact_layers = 0;
  double marker_safety_factor = 0.0;
  std::string leaf_count_by_level;
  std::size_t contact_band_block_count = 0;
  std::size_t far_field_block_count = 0;
  std::size_t contact_band_cell_count = 0;
  std::size_t exact_node_count = 0;
  std::size_t predicted_node_count = 0;
  double exact_node_ratio = 0.0;
  bool mask_changed_vs_baseline = false;
  std::size_t new_exact_node_count = 0;
  std::size_t removed_exact_node_count = 0;
  double jaccard_similarity_vs_baseline = 1.0;
  double marker_time_ms = 0.0;
};

void usage() {
  std::cerr
      << "Usage: adasdf_diagnose_contact_band_mask model.stl "
         "[--max-level N] [--block-resolution N] "
         "[--adaptive-leaf-mode mixed|uniform] "
         "[--contact-band-widths a,b,c] "
         "[--halo-exact-layers-list a,b,c] "
         "[--marker-safety-factors a,b,c] "
         "[--contact-band-marker conservative-aabb|distance-aware|hybrid] "
         "[--marker-cell-size-factor value] [--local-halo-only] "
         "[--json out.json] [--csv out.csv] [--report out.md]\n";
}

bool hasValue(int i, int argc) {
  return i + 1 < argc;
}

std::vector<double> parseDoubles(const std::string& text) {
  std::vector<double> values;
  std::stringstream stream(text);
  std::string item;
  while (std::getline(stream, item, ',')) {
    if (!item.empty()) {
      values.push_back(std::stod(item));
    }
  }
  return values;
}

std::vector<int> parseInts(const std::string& text) {
  std::vector<int> values;
  std::stringstream stream(text);
  std::string item;
  while (std::getline(stream, item, ',')) {
    if (!item.empty()) {
      values.push_back(std::stoi(item));
    }
  }
  return values;
}

bool parseArgs(int argc, char** argv, Options* options) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      usage();
      std::exit(0);
    } else if (arg == "--max-level" && hasValue(i, argc)) {
      options->max_level = std::stoi(argv[++i]);
    } else if (arg == "--min-level" && hasValue(i, argc)) {
      options->min_level = std::stoi(argv[++i]);
    } else if (arg == "--block-resolution" && hasValue(i, argc)) {
      options->block_resolution = std::stoi(argv[++i]);
    } else if (arg == "--adaptive-leaf-mode" && hasValue(i, argc)) {
      if (!adasdf::parseAdaptiveLeafMode(
              argv[++i],
              &options->adaptive_leaf_mode)) {
        return false;
      }
    } else if (arg == "--contact-band-widths" && hasValue(i, argc)) {
      options->contact_band_widths = parseDoubles(argv[++i]);
    } else if (arg == "--halo-exact-layers-list" && hasValue(i, argc)) {
      options->halo_exact_layers = parseInts(argv[++i]);
    } else if (arg == "--marker-safety-factors" && hasValue(i, argc)) {
      options->marker_safety_factors = parseDoubles(argv[++i]);
    } else if (arg == "--contact-band-marker" && hasValue(i, argc)) {
      if (!adasdf::parseContactBandMarkerMode(
              argv[++i],
              &options->marker_mode)) {
        return false;
      }
    } else if (arg == "--marker-cell-size-factor" && hasValue(i, argc)) {
      options->marker_cell_size_factor = std::stod(argv[++i]);
    } else if (arg == "--local-halo-only") {
      options->local_halo_only = true;
    } else if (arg == "--distance-backend" && hasValue(i, argc)) {
      ++i;
    } else if (arg == "--threads" && hasValue(i, argc)) {
      ++i;
    } else if (arg == "--json" && hasValue(i, argc)) {
      options->json = argv[++i];
    } else if (arg == "--csv" && hasValue(i, argc)) {
      options->csv = argv[++i];
    } else if (arg == "--report" && hasValue(i, argc)) {
      options->report = argv[++i];
    } else if (!arg.empty() && arg[0] == '-') {
      std::cerr << "Unknown argument: " << arg << "\n";
      return false;
    } else if (options->stl.empty()) {
      options->stl = arg;
    } else {
      std::cerr << "Unexpected positional argument: " << arg << "\n";
      return false;
    }
  }
  return !options->stl.empty() && !options->contact_band_widths.empty() &&
         !options->halo_exact_layers.empty() &&
         !options->marker_safety_factors.empty();
}

std::string leafCounts(const adasdf::AdaptiveTreeStats& stats) {
  std::ostringstream out;
  bool first = true;
  for (const adasdf::AdaptiveTreeLevelStats& level : stats.levels) {
    if (!first) {
      out << ";";
    }
    first = false;
    out << level.level << ":" << level.leaf_block_count;
  }
  return out.str();
}

std::set<std::string> exactSetFor(
    const adasdf::AdaptiveSDFBlockSet& blocks,
    const adasdf::TriangleBVH& bvh,
    const adasdf::ContactBandOptions& options,
    Row* row) {
  std::set<std::string> exact;
  for (const adasdf::AdaptiveSDFBlock& block : blocks.blocks) {
    const adasdf::ContactBandMask mask = adasdf::ContactBandMarker::markBlock(
        block.bounds,
        block.nx,
        bvh,
        options,
        nullptr,
        block.block_id,
        block.level);
    row->marker_time_ms += mask.marker_time_ms;
    row->contact_band_cell_count += mask.marked_cell_count;
    if (mask.contact_band_node_count > 0) {
      ++row->contact_band_block_count;
    } else {
      ++row->far_field_block_count;
    }
    for (std::size_t i = 0; i < mask.exact_required.size(); ++i) {
      if (mask.exact_required[i] != 0) {
        exact.insert(std::to_string(block.block_id) + ":" + std::to_string(i));
      }
    }
  }
  return exact;
}

std::vector<Row> runDiagnosis(
    const adasdf::TriangleMesh& mesh,
    const Options& options) {
  adasdf::AdaptiveOctreeBuildOptions octree_options;
  octree_options.min_level = options.min_level;
  octree_options.max_level = options.max_level;
  octree_options.leaf_mode = options.adaptive_leaf_mode;
  octree_options.distance_backend = adasdf::SDFSamplingAcceleration::BVH;
  adasdf::AdaptiveOctreeBuildReport octree_report;
  const adasdf::AdaptiveOctree octree =
      adasdf::AdaptiveOctreeBuilder::build(mesh, octree_options, &octree_report);
  adasdf::AdaptiveBlockPartitionOptions partition_options;
  partition_options.block_resolution = options.block_resolution;
  const adasdf::AdaptiveSDFBlockSet blocks =
      adasdf::AdaptiveBlockPartitioner::partition(octree, partition_options);
  const adasdf::AdaptiveTreeStats tree_stats =
      adasdf::computeAdaptiveTreeStats(
          octree,
          blocks,
          options.block_resolution,
          options.max_level);
  adasdf::TriangleBVHBuildReport bvh_report;
  const adasdf::TriangleBVH bvh =
      adasdf::TriangleBVHBuilder::build(mesh, {}, &bvh_report);

  std::vector<Row> rows;
  std::set<std::string> baseline;
  bool have_baseline = false;
  for (double width : options.contact_band_widths) {
    for (int halo : options.halo_exact_layers) {
      for (double safety : options.marker_safety_factors) {
        Row row;
        std::ostringstream id;
        id << "band" << width << "_halo" << halo << "_safety" << safety;
        row.case_id = id.str();
        row.contact_band_width = width;
        row.halo_exact_layers = halo;
        row.marker_safety_factor = safety;
        row.leaf_count_by_level = leafCounts(tree_stats);

        adasdf::ContactBandOptions marker_options;
        marker_options.contact_band_width = width;
        marker_options.contact_band_layers = 1;
        marker_options.halo_exact_layers = halo;
        marker_options.marker_mode = options.marker_mode;
        marker_options.marker_cell_size_factor = options.marker_cell_size_factor;
        marker_options.marker_safety_factor = safety;
        marker_options.local_halo_only = options.local_halo_only;
        marker_options.disable_global_halo = options.local_halo_only;
        const std::set<std::string> exact =
            exactSetFor(blocks, bvh, marker_options, &row);
        const std::size_t logical =
            blocks.blocks.size() *
            adasdf::adaptiveLogicalNodesPerBlock(options.block_resolution);
        row.exact_node_count = exact.size();
        row.predicted_node_count =
            logical > row.exact_node_count ? logical - row.exact_node_count : 0;
        row.exact_node_ratio =
            logical == 0 ? 0.0
                         : static_cast<double>(row.exact_node_count) /
                               static_cast<double>(logical);
        if (!have_baseline) {
          baseline = exact;
          have_baseline = true;
        } else {
          std::size_t intersection = 0;
          for (const std::string& key : exact) {
            if (baseline.count(key) != 0) {
              ++intersection;
            } else {
              ++row.new_exact_node_count;
            }
          }
          for (const std::string& key : baseline) {
            if (exact.count(key) == 0) {
              ++row.removed_exact_node_count;
            }
          }
          const std::size_t union_count =
              exact.size() + baseline.size() - intersection;
          row.jaccard_similarity_vs_baseline =
              union_count == 0
                  ? 1.0
                  : static_cast<double>(intersection) /
                        static_cast<double>(union_count);
          row.mask_changed_vs_baseline =
              row.new_exact_node_count != 0 || row.removed_exact_node_count != 0;
        }
        rows.push_back(row);
      }
    }
  }
  return rows;
}

void createParent(const std::filesystem::path& path) {
  if (!path.empty() && path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }
}

bool writeCSV(const std::filesystem::path& path, const std::vector<Row>& rows) {
  createParent(path);
  std::ofstream out(path);
  if (!out) {
    return false;
  }
  out << "case_id,contact_band_width,halo_exact_layers,"
         "marker_safety_factor,leaf_count_by_level,contact_band_block_count,"
         "far_field_block_count,contact_band_cell_count,exact_node_count,"
         "predicted_node_count,exact_node_ratio,mask_changed_vs_baseline,"
         "new_exact_node_count,removed_exact_node_count,"
         "jaccard_similarity_vs_baseline,marker_time_ms\n";
  out << std::setprecision(17);
  for (const Row& row : rows) {
    out << row.case_id << "," << row.contact_band_width << ","
        << row.halo_exact_layers << "," << row.marker_safety_factor << ","
        << row.leaf_count_by_level << "," << row.contact_band_block_count
        << "," << row.far_field_block_count << ","
        << row.contact_band_cell_count << "," << row.exact_node_count << ","
        << row.predicted_node_count << "," << row.exact_node_ratio << ","
        << (row.mask_changed_vs_baseline ? "true" : "false") << ","
        << row.new_exact_node_count << "," << row.removed_exact_node_count
        << "," << row.jaccard_similarity_vs_baseline << ","
        << row.marker_time_ms << "\n";
  }
  return true;
}

bool writeJson(const std::filesystem::path& path, const std::vector<Row>& rows) {
  createParent(path);
  std::ofstream out(path);
  if (!out) {
    return false;
  }
  out << std::setprecision(17);
  out << "{\n  \"schema_id\": \"adasdf.contact_band_mask_sensitivity.v1\",\n";
  out << "  \"cases\": [\n";
  for (std::size_t i = 0; i < rows.size(); ++i) {
    const Row& row = rows[i];
    out << "    {\"case_id\": \"" << row.case_id << "\""
        << ", \"contact_band_width\": " << row.contact_band_width
        << ", \"halo_exact_layers\": " << row.halo_exact_layers
        << ", \"marker_safety_factor\": " << row.marker_safety_factor
        << ", \"leaf_count_by_level\": \"" << row.leaf_count_by_level << "\""
        << ", \"contact_band_block_count\": " << row.contact_band_block_count
        << ", \"far_field_block_count\": " << row.far_field_block_count
        << ", \"contact_band_cell_count\": " << row.contact_band_cell_count
        << ", \"exact_node_count\": " << row.exact_node_count
        << ", \"predicted_node_count\": " << row.predicted_node_count
        << ", \"exact_node_ratio\": " << row.exact_node_ratio
        << ", \"mask_changed_vs_baseline\": "
        << (row.mask_changed_vs_baseline ? "true" : "false")
        << ", \"new_exact_node_count\": " << row.new_exact_node_count
        << ", \"removed_exact_node_count\": " << row.removed_exact_node_count
        << ", \"jaccard_similarity_vs_baseline\": "
        << row.jaccard_similarity_vs_baseline
        << ", \"marker_time_ms\": " << row.marker_time_ms << "}";
    if (i + 1 < rows.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ]\n}\n";
  return true;
}

bool writeMarkdown(
    const std::filesystem::path& path,
    const std::vector<Row>& rows) {
  createParent(path);
  std::ofstream out(path);
  if (!out) {
    return false;
  }
  out << std::setprecision(10);
  out << "# Contact-Band Mask Sensitivity\n\n";
  out << "| case | band | halo | safety | leaves | contact blocks | far blocks | cells | exact | predicted | exact ratio | changed | new exact | removed exact | jaccard | marker ms |\n";
  out << "| --- | ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: |\n";
  for (const Row& row : rows) {
    out << "| " << row.case_id << " | " << row.contact_band_width << " | "
        << row.halo_exact_layers << " | " << row.marker_safety_factor
        << " | `" << row.leaf_count_by_level << "` | "
        << row.contact_band_block_count << " | " << row.far_field_block_count
        << " | " << row.contact_band_cell_count << " | "
        << row.exact_node_count << " | " << row.predicted_node_count << " | "
        << row.exact_node_ratio << " | "
        << (row.mask_changed_vs_baseline ? "true" : "false") << " | "
        << row.new_exact_node_count << " | " << row.removed_exact_node_count
        << " | " << row.jaccard_similarity_vs_baseline << " | "
        << row.marker_time_ms << " |\n";
  }
  return true;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    Options options;
    if (!parseArgs(argc, argv, &options)) {
      usage();
      return 2;
    }
    const adasdf::STLReadResult read =
        adasdf::STLReader::read(options.stl.string());
    if (!read.success) {
      std::cerr << "failed to read STL: " << read.error_message << "\n";
      return 1;
    }
    const std::vector<Row> rows = runDiagnosis(read.mesh, options);
    if (!options.json.empty() && !writeJson(options.json, rows)) {
      std::cerr << "failed to write JSON\n";
      return 1;
    }
    if (!options.csv.empty() && !writeCSV(options.csv, rows)) {
      std::cerr << "failed to write CSV\n";
      return 1;
    }
    if (!options.report.empty() && !writeMarkdown(options.report, rows)) {
      std::cerr << "failed to write report\n";
      return 1;
    }
    std::cout << "contact-band mask sensitivity completed\n";
    std::cout << "case_count=" << rows.size() << "\n";
    if (!rows.empty()) {
      std::cout << "baseline_exact_node_count="
                << rows.front().exact_node_count << "\n";
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_diagnose_contact_band_mask failed: " << exc.what()
              << "\n";
    return 1;
  }
}
