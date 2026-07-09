#pragma once

#include "adasdf/cache/SDFSampleCache.h"

namespace adasdf {

class SignQueryCache {
 public:
  explicit SignQueryCache(SDFSampleCache* sample_cache = nullptr);

  bool find(const QuantizedPointKey& key, int* sign) const;
  void insert(const QuantizedPointKey& key, int sign);

 private:
  SDFSampleCache* sample_cache_ = nullptr;
};

}  // namespace adasdf
