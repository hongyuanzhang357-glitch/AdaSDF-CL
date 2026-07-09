#pragma once

#include "adasdf/cache/SDFSampleCache.h"

namespace adasdf {

class DistanceQueryCache {
 public:
  explicit DistanceQueryCache(SDFSampleCache* sample_cache = nullptr);

  bool find(const QuantizedPointKey& key, double* phi) const;
  void insert(const QuantizedPointKey& key, double phi);

 private:
  SDFSampleCache* sample_cache_ = nullptr;
};

}  // namespace adasdf
