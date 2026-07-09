# Adaptive Build Sample Cache

Release: `v1.17.2-alpha`

AdaSDF-CL v1.17.2-alpha adds build-time caches for repeated adaptive SDF
sampling work. The caches are CPU-only and are used during STL-to-SDF
construction; they do not change `.sdfbin` file formats or runtime query
interfaces.

## Purpose

Adaptive block construction can revisit the same geometric point through
neighboring cells, block halos, contact-band exact nodes, and repeated sign or
distance queries. The sample cache stores exact SDF samples behind a
deterministic quantized point key so repeated work can be skipped.

The cache path is conservative:

- cache keys are integer lattice keys, not raw `double` strings;
- `--sample-cache off` keeps the no-cache reference path available;
- block-local cache is the default safe scope;
- global cache is explicit because it can increase memory use;
- quality and speedup claims should be compared against a no-cache reference.

## CLI Flags

Build and benchmark tools accept:

```text
--sample-cache off|block|global
--corner-cache off|block|global
--sign-cache off|on
--distance-cache off|on
--marker-cache off|on
--cache-max-entries N
--cache-quantization-epsilon value
--report-cache-stats
```

`block` means the cache is scoped to the current block or local build unit.
`global` enables cross-block reuse and should be profiled for memory impact.

## Cache Modules

- `QuantizedPointKey`: converts points into deterministic integer keys.
- `SDFSampleCache`: stores full phi/sign/nearest-triangle samples.
- `DistanceQueryCache` and `SignQueryCache`: light wrappers for separate
  distance/sign accounting.
- `CellCornerCache`: reuses grid or adaptive cell corner phi values.
- `BlockPointDeduplicator`: batches exact-required node positions and maps
  duplicate positions back to original nodes.
- `AdaptiveRefinementCache`: reusable cache facade for adaptive refinement.
- `ContactBandMarkerCache`: caches marker cell decisions and candidate
  triangle lists with a marker configuration id.

## Build Profile Fields

`adasdf.build_profile.v1` keeps the v1 schema and adds optional fields under
`timings` and `counters`, including:

- `cache_lookup_time_ms`
- `cache_insert_time_ms`
- `deduplication_time_ms`
- `marker_cache_time_ms`
- `sample_cache_enabled`
- `sample_cache_scope`
- `sample_cache_hits`
- `sample_cache_misses`
- `distance_queries_saved`
- `sign_queries_saved`
- `block_point_duplicate_count`
- `marker_decision_cache_hits`
- `box_triangle_distance_saved`

These fields indicate whether the cache is active and whether it reduced
repeated work. They are not by themselves a speedup claim.

## Correctness Boundary

Cache-enabled builds must be compared with `--sample-cache off` reference
builds. The new `adasdf_benchmark_build_cache` tool reports phi differences,
sign mismatches, `quality_passed`, and `performance_claim_allowed` in the same
table as speedup and query-count fields.

No CUDA feature, FCL-style API, GPU-native compressed query path, or `.sdfbin`
format change is introduced in this release.
