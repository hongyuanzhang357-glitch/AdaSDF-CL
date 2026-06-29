#pragma once

#include "adasdf/geometry/SDFModel.h"
#include "adasdf/query/ExpandedSDF.h"
#include "adasdf/query/QueryModeConfig.h"

namespace adasdf {

struct ExpansionOptions {
  QueryExpansionMode expansion = QueryExpansionMode::Global;
  BlockSelection block_selection = BlockSelection::all();

  int global_resolution = 64;
  int block_resolution = 32;

  bool include_distance_values = true;
  bool include_gradient_values = false;

  double padding = 0.0;
};

class SDFExpander {
 public:
  static ExpandedSDF expand(
      const SDFModel& model,
      const ExpansionOptions& options);

  static ExpandedSDF expandGlobal(
      const SDFModel& model,
      int resolution);

  static ExpandedSDF expandBlocks(
      const SDFModel& model,
      const BlockSelection& blocks,
      int block_resolution);
};

}  // namespace adasdf
