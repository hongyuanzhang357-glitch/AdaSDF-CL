# v1.17.2 Adaptive Cache Report

## Goal

AdaSDF-CL v1.17.2-alpha reduces repeated build-time SDF work during adaptive
refinement, block sampling, and contact-band construction while preserving
existing model formats and no-cache reference behavior.

## Implemented

- `include/adasdf/cache` and `src/cache` modules for quantized keys, SDF sample
  caching, corner caching, point deduplication, adaptive refinement cache, and
  contact-band marker cache.
- Build CLI cache flags on dense, adaptive, compressed, and standalone
  compression tools.
- Cache option parsing on hierarchical and contact-band benchmark tools.
- Build profile counters/timings for cache activity.
- `adasdf_benchmark_build_cache` with no-cache reference comparison.
- Python wrapper cache flags and `benchmark_build_cache`.
- CPU-only tests for cache data structures, profile fields, CLI flags, and
  Python wrappers.

## Correctness

Cache-enabled paths are intended to preserve phi/sign output relative to
`--sample-cache off`. `adasdf_benchmark_build_cache` reports:

- `max_abs_phi_diff`
- `rms_phi_diff`
- `p95_phi_diff`
- `sign_mismatch_count`
- `quality_passed`
- `performance_claim_allowed`

## Performance Semantics

`distance_queries_saved`, `sign_queries_saved`, cache hit rates, and duplicate
point counts explain repeated work reduction. A valid speedup statement should
cite `speedup_vs_no_cache` together with `quality_passed` and
`sign_mismatch_count`.

## Non-Goals

- No `.sdfbin` format migration.
- No GPU-native compressed SDF path.
- No FCL adapter expansion.
- No dynamic CollisionWorld work.
- No external model downloads.
