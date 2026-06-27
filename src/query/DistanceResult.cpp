#include "adasdf/query/DistanceResult.h"

namespace adasdf {

void DistanceResult::clear() {
  has_result_ = false;
  minimum_distance_ = 0.0;
  nearest_point_a_ = {};
  nearest_point_b_ = {};
  normal_ = {};
  closest_contact_ = {};
  sdf_query_count_ = 0;
  candidate_point_count_ = 0;
  raw_contact_count_ = 0;
  reduced_contact_count_ = 0;
  backend_info_.clear();
  method_info_.clear();
}

}  // namespace adasdf
