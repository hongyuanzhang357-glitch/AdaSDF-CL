#pragma once

#include "adasdf/geometry/AnalyticSDFModel.h"
#include "adasdf/geometry/DemoAdaptiveSDFModel.h"
#include "adasdf/query/BatchQuery.h"

namespace adasdf {

class CudaQueryBackend {
 public:
  static bool isAvailable();

  static BatchQueryOutput queryAnalyticBox(
      const AnalyticSDFModel& model,
      const BatchQueryInput& input);

  static BatchQueryOutput queryDemoAdaptiveBox(
      const DemoAdaptiveSDFModel& model,
      const BatchQueryInput& input);
};

}  // namespace adasdf
