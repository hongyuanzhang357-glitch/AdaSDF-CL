# AdaSDF-CL v1.1.0-alpha Expanded SDF Accuracy Report

## Goal

Move from benchmark speed semantics to expanded-SDF reliability metrics:
direct-vs-expanded error, sign mismatch, ambiguous sign, near-surface risk,
fallback behavior, and sampled existing-core expansion.

## Added Files

- `include/adasdf/query/ExpansionQuality.h`
- `src/query/ExpansionQuality.cpp`
- `tools/adasdf_expansion_quality.cpp`
- `tests/test_expansion_quality.cpp`
- `tests/test_sign_mismatch_metrics.cpp`
- `tests/test_expansion_resolution_control.cpp`
- `tests/test_existing_core_expansion_bridge.cpp`
- `tests/test_expansion_quality_cli.cpp`
- `docs/expanded_sdf_accuracy.md`
- `docs/sign_mismatch_metrics.md`
- `docs/existing_core_expansion_bridge.md`
- `docs/github_release_draft_v1_1_0_alpha.md`
- `docs/expanded_sdf_accuracy_v1_1_report.md`

## Accuracy Controls

`ExpansionOptions` now includes global/block resolution, padding,
near-surface band, sign epsilon, quality-audit sample count, clamp behavior,
and direct fallback policy.

## Sign Definitions

```text
inside:    phi < -sign_epsilon
outside:   phi > +sign_epsilon
ambiguous: abs(phi) <= sign_epsilon
```

Strict sign mismatch counts only inside-vs-outside disagreement. Ambiguous
samples are tracked separately. Near-surface sign mismatch is computed over
samples where `abs(phi_direct) <= near_surface_band`.

## Quality Results

Demo adaptive box, 10000 samples:

| Expansion | Resolution | Max abs error | Mean abs error | RMS error | P95 abs error | Sign mismatch rate | Ambiguous rate | Near-surface sign mismatch rate | Fallback count |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| global | 64 | 0.0040849 | 0.0000356537 | 0.000267448 | 0.0000000000 | 0 | 0.2577 | 0 | 0 |
| block all | 32 | 0.0109941 | 0.000147058 | 0.000786323 | 0.000408893 | 0 | 0.2577 | 0 | 0 |

Resolution control test compares global 16 vs global 32 and asserts that the
finer grid does not clearly worsen mean or p95 error for deterministic samples.

## Benchmark Quality Fields

`adasdf_benchmark_batch_query` now writes:

```text
max_abs_error,mean_abs_error,rms_error,p95_abs_error,
sign_mismatch_count,sign_mismatch_rate,ambiguous_sign_count,
ambiguous_sign_rate,near_surface_sign_mismatch_count,
near_surface_sign_mismatch_rate,fallback_rate
```

CPU global and CPU block benchmark smoke rows produced finite quality values and
zero strict sign mismatches for the demo adaptive box.

## Existing-Core Bridge Status

The sampled bridge is available through `SDFBinReader -> SDFModel direct query
-> SDFExpander -> ExpandedSDF -> ExpansionQuality`. Existing-core fixtures are
optional in CI and tests skip gracefully when the existing core is unavailable.

## Current Limits

- ExpandedSDF is an approximation of direct query.
- Block expansion is best for local contact regions, not arbitrary global
  uniform point clouds.
- CUDA remains optional.
- v1.1 does not implement full low-rank compressed SDF GPU-native query.

## Next Steps

- Add richer point sampling around true mesh surfaces when mesh topology is
  available.
- Add per-block quality summaries.
- Expose quality thresholds in CI smoke tests without making them brittle.
- Continue native low-rank GPU query as future work.
