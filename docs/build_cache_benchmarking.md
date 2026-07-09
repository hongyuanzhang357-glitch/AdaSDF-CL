# Build Cache Benchmarking

Release: `v1.17.2-alpha`

`adasdf_benchmark_build_cache` compares no-cache, block-cache, and optional
global-cache SDF builds using the same input mesh and build parameters.

## Example

```powershell
adasdf_benchmark_build_cache model.stl `
  --builder compressed `
  --max-level 4 `
  --block-resolution 8 `
  --target-error 1e-3 `
  --max-rank 8 `
  --sampling contact-band `
  --cache-modes off,block,global `
  --distance-backend bvh `
  --csv build_cache.csv `
  --report build_cache.md
```

The tool always requires or inserts an `off` run so cache-enabled rows have a
reference.

## Output Fields

The CSV includes:

- `speedup_vs_no_cache`
- `num_distance_queries_no_cache`
- `num_distance_queries_cache`
- `num_sign_queries_no_cache`
- `num_sign_queries_cache`
- `sample_cache_hit_rate`
- `corner_cache_hit_rate`
- `sign_cache_hit_rate`
- `distance_queries_saved`
- `sign_queries_saved`
- `duplicate_point_count`
- `marker_cache_hit_rate`
- `max_abs_phi_diff`
- `rms_phi_diff`
- `p95_phi_diff`
- `sign_mismatch_count`
- `quality_passed`
- `performance_claim_allowed`

`performance_claim_allowed` is true only when `speedup_vs_no_cache > 1`,
quality passes, and the sign mismatch count is zero.

## Interpretation

Use cache hit rates and saved query counts to explain where time changed. A
speedup without `quality_passed=true` should not be reported as a valid
performance improvement.

Global cache can reduce cross-block repeated work but may increase memory and
lock contention. Block cache is the conservative default.
