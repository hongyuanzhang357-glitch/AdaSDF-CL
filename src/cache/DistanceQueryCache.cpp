#include "adasdf/cache/DistanceQueryCache.h"

namespace adasdf {

DistanceQueryCache::DistanceQueryCache(SDFSampleCache* sample_cache)
    : sample_cache_(sample_cache) {}

bool DistanceQueryCache::find(const QuantizedPointKey& key, double* phi) const {
  if (sample_cache_ == nullptr) {
    return false;
  }
  CachedSDFSample sample;
  if (!sample_cache_->find(key, &sample)) {
    return false;
  }
  if (phi != nullptr) {
    *phi = sample.phi;
  }
  return true;
}

void DistanceQueryCache::insert(const QuantizedPointKey& key, double phi) {
  if (sample_cache_ == nullptr) {
    return;
  }
  CachedSDFSample sample;
  sample.phi = phi;
  sample.finite = true;
  sample_cache_->insert(key, sample);
}

}  // namespace adasdf
