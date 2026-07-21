#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>

int main() {
  try {
    adasdf::AdaptiveTreeStats stats;
    stats.min_level = 1;
    stats.max_level = 3;
    stats.max_level_used = 2;
    stats.block_resolution = 5;
    stats.leaf_block_count = 9;
    stats.total_logical_node_count = 1125;
    stats.uniform_max_level_leaf_count = 512;
    stats.uniform_max_level_logical_node_count = 64000;
    stats.sparsity_ratio_vs_uniform_max_level = 0.017578125;
    stats.mixed_level_present = true;
    adasdf::AdaptiveTreeLevelStats level;
    level.level = 2;
    level.leaf_block_count = 8;
    level.logical_node_count = 1000;
    stats.levels.push_back(level);

    const std::string json =
        adasdf::AdaptiveTreeStatsWriter::toJson(stats, "unit");
    const std::string md = adasdf::AdaptiveTreeStatsWriter::toMarkdown(stats);
    if (json.find("\"schema_id\":\"adasdf.adaptive_tree_stats.v1\"") ==
            std::string::npos ||
        json.find("\"sparsity_ratio_vs_uniform_max_level\"") ==
            std::string::npos ||
        json.find("\"mixed_level_present\":true") == std::string::npos ||
        md.find("AdaSDF-CL Adaptive Tree Diagnostics") ==
            std::string::npos ||
        md.find("Sparsity ratio vs uniform max-level") ==
            std::string::npos) {
      std::cerr << "adaptive tree stats writer output missing fields\n";
      return 1;
    }
    std::cout << "adaptive tree stats writer passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_adaptive_tree_stats_writer failed: " << exc.what()
              << "\n";
    return 1;
  }
}
