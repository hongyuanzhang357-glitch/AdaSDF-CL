# AdaSDF-CL v1.17.2-alpha

Adaptive build sample cache and repeated query reduction.

## Added

- Quantized point keys for deterministic build-time cache lookup.
- SDF sample cache, distance cache, and sign cache modules.
- Cell corner cache for adaptive and block sampling paths.
- Block-local point deduplication for exact-required nodes.
- Adaptive refinement cache facade.
- Contact-band marker cache with marker-parameter configuration ids.
- Build profile cache counters and cache timings.
- `adasdf_benchmark_build_cache` for cache off/on benchmarking against a
  no-cache reference.
- Python wrapper options for build cache flags and the build cache benchmark.

## Improved

- Reduces repeated build-time SDF sampling work in adaptive/contact-band build
  paths.
- Reports cache hits, misses, saved distance/sign queries, duplicate point
  counts, and marker cache counters in build profiles.
- Keeps cache speedup claims tied to no-cache quality comparison.

## Boundaries

- No `.sdfbin` format change.
- No new CUDA feature.
- No FCL-style API change.
- No GPU-native compressed SDF query.
- Global cache is optional and may increase memory.
- Performance claims require `quality_passed=true` and zero sign mismatches in
  the build cache benchmark.
