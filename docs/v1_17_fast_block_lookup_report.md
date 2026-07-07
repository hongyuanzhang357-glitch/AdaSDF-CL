# AdaSDF-CL v1.17.0-alpha Fast Block Lookup Report

## Goal

v1.17.0-alpha optimizes runtime block lookup and active block cache lookup for
adaptive/compressed SDF sparse and contact workflows.

## Implemented Components

- `include/adasdf/lookup/MortonKey.h`
- `include/adasdf/lookup/BlockSpatialKey.h`
- `include/adasdf/lookup/BlockLookupIndex.h`
- `include/adasdf/lookup/ActiveBlockHashMap.h`
- `include/adasdf/lookup/ActiveBlockSpatialHash.h`
- `include/adasdf/lookup/CacheSlotMap.h`
- `include/adasdf/lookup/BlockLookupDiagnostics.h`
- `include/adasdf/lookup/BlockLookupReportWriter.h`
- `tools/adasdf_benchmark_block_lookup.cpp`

## Query Semantics

The lookup layer finds the containing block or cache slot. It does not change
the interpolation math inside a block. The existing block-local trilinear
interpolation path still computes the floor cell and eight flattened regular
grid indices directly.

## Consistency Rule

Linear scan remains the reference. Fast lookup performance claims are allowed
only when:

```text
speedup_vs_linear > 1
lookup_result_mismatch_count == 0
phi_mismatch_count == 0 or max_abs_phi_diff <= tolerance
```

## CUDA Readiness

`CudaActiveBlockCache` now exposes flat lookup metadata for future GPU lookup,
but v1.17.0-alpha does not implement a GPU hash table.

## Release Benchmark

The release benchmark is generated under the external validation workroot. The
canonical artifact names are:

- `benchmarks/v1_17_block_lookup_cube.csv`
- `reports/v1_17_block_lookup_cube.md`

Do not commit generated benchmark CSV, `.sdfbin`, build, install, or log
artifacts.

The requested max-level 5 contact-band compressed cube build was attempted
locally first, but it stayed silent for several minutes and was stopped to
avoid blocking the release gate. The lookup benchmark below uses the same cube
fixture and the same compressed builder in a smaller smoke configuration:

```text
max_level: 3
block_resolution: 8
block_count: 512
sampling_mode: exact
```

Best representative row from the local release benchmark:

```text
lookup_mode: hash
cache_lookup_mode: spatial-hash
sample_count: 6
block_count: 512
active_block_count: 512
speedup_vs_linear: 1.76404
lookup_result_mismatch_count: 0
phi_mismatch_count: 0
max_abs_phi_diff: 0
cache_hit_rate: 1
linear_fallback_count: 22
missed_lookup_count: 22
performance_claim_allowed: true
```

The sparse cube sample fixture contains only six samples, so this benchmark is
a release smoke and consistency check rather than a broad performance claim.

## Validation Snapshot

- CPU-only CMake configure/build: PASS.
- CPU CTest: 216/216 PASS.
- Install validation: PASS.
- Alpha validation: PASS.
- Python wrapper unittest: 60/60 PASS.
- CUDA unavailable behavior: skipped where expected.
