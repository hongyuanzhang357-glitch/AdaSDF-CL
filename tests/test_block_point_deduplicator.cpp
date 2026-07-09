#include <adasdf/adasdf.h>

#include <iostream>
#include <vector>

int main() {
  adasdf::QuantizationOptions options;
  options.origin = {0.0, 0.0, 0.0};
  options.spacing = 0.5;
  options.epsilon = 1e-9;
  const std::vector<adasdf::Vector3> points{
      {0.0, 0.0, 0.0},
      {1e-13, -1e-13, 0.0},
      {0.5, 0.0, 0.0},
      {0.5 + 1e-13, 0.0, 0.0}};
  const adasdf::DeduplicatedPointSet dedup =
      adasdf::BlockPointDeduplicator::deduplicate(points, 2, options);
  if (dedup.unique_points.size() != 2 || dedup.keys.size() != 2 ||
      dedup.reverse_index.size() != points.size() ||
      dedup.duplicate_count != 2) {
    std::cerr << "unexpected block point deduplication result\n";
    return 1;
  }
  if (dedup.reverse_index[0] != dedup.reverse_index[1] ||
      dedup.reverse_index[2] != dedup.reverse_index[3] ||
      dedup.reverse_index[0] == dedup.reverse_index[2]) {
    std::cerr << "dedup reverse map is inconsistent\n";
    return 1;
  }
  return 0;
}
