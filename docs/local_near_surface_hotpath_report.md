# AdaSDF-CL Local Near-Surface Exact Hot-Path Report

Local build id: `1.16.0-alpha.2-local-hotpath`

This is a local performance sprint report, not a public release draft. The
public version string, public tags, and GitHub release notes remain unchanged
at `1.16.0-alpha.2`.

## Goal

v1.16.0-alpha.2 showed that hierarchical sampling can reduce exact BVH sample
count while preserving quality, but the wall time was still slower than the
exact reference on the cube L5 fixture. This sprint diagnosed whether the extra
time came from the hierarchical exact path itself, then prototyped a local
near-surface banded prediction path.

## Alpha.2 Problem Recap

The alpha.2-equivalent cube L5 benchmark preserved quality:

- `max_abs_error = 0`
- `rms_error = 0`
- `p95_error = 0`
- `sign_mismatch_count = 0`
- `near_surface_sign_mismatch_count = 0`
- `fallback_block_count = 0`

But speedup was below 1:

- exact build time: `18125.2 ms`
- hierarchical build time: `38868.9 ms`
- speedup: `0.466316`

The main cause was near-surface exact-dominated sampling: most near-surface
blocks still needed dense exact BVH sampling, and the hierarchical exact path
had higher per-sample overhead than the reference exact path.

## Exact Hotpath Microbenchmark

Fixture: `closed_cube_ascii.stl`, max level `5`, block resolution `8`,
threads `2`.

| Metric | Value |
| --- | ---: |
| block_count | 29128 |
| sample_count | 14913536 |
| reference_exact_time_ms | 17311.1 |
| hierarchical_exact_time_ms | 52616.4 |
| optimized_hierarchical_exact_time_ms | 22250.2 |
| reference_ns_per_sample | 1160.76 |
| hierarchical_ns_per_sample | 3528.09 |
| optimized_hierarchical_ns_per_sample | 1491.94 |
| hierarchical_overhead_ratio | 3.03946 |
| optimized_overhead_ratio | 1.28531 |
| distance_query_count | 14913536 |
| sign_query_count | 14913536 |
| allocation_count_estimate | 87385 |

The optimized exact hot path improved the ratio from `3.03946x` to
`1.28531x`, but did not meet the target
`optimized_hierarchical_ns_per_sample <= 1.10 * reference_ns_per_sample`.
The remaining gap appears to be scheduling/allocation/locality overhead versus
the global reference sample-location loop.

## Overhead Fixes

Implemented locally:

- A unified `ExactBlockSamplerKernel` used by hierarchical exact block sampling.
- Fast exact sampling path when counters and diagnostics are disabled.
- Bulk near-surface exact sampling in the adaptive builder when detailed
  hierarchical diagnostics are disabled.
- Parallel block sampling in the no-diagnostics path; diagnostics still use
  conservative aggregation.
- Precomputed local fallback masks for banded near-surface prediction.
- CLI/report fields for near-surface banded diagnostics and far-field sign
  policy.

The no-diagnostics cube L5 exact-mode benchmark improved materially:

- alpha.2-equivalent diagnostics-on speedup: `0.466316`
- optimized exact no-diagnostics speedup: `0.94956`

This confirms that much of the alpha.2 slowdown was measurement/diagnostic and
per-block exact-path overhead, not a quality issue.

## Near-Surface Banded Prediction

The experimental `--near-surface-mode banded` path keeps halo layers and
near-band nodes exact while predicting clearly farther fine nodes inside a
near-surface block. The default remains `exact`.

### Cube L5 Banded

| Metric | Value |
| --- | ---: |
| exact_build_time_ms | 18608.4 |
| hierarchical_build_time_ms | 31091 |
| speedup | 0.598514 |
| exact_bvh_sample_count | 8481600 |
| predicted_sample_count | 7451416 |
| quality_check_sample_count | 233024 |
| local_fallback_node_count | 7462120 |
| near_surface_local_exact_node_count | 7462120 |
| near_surface_predicted_node_count | 3863320 |
| local_prediction_acceptance_rate | 0.341119 |
| max_abs_error | 0 |
| rms_error | 0 |
| p95_error | 0 |
| sign_mismatch_count | 0 |
| near_surface_sign_mismatch_count | 0 |
| quality_gate_passed | true |

Cube L5 banded reduced exact BVH samples from `11570720` to `8481600`, but the
additional coarse sampling, prediction, quality checks, and local fallback
exact sampling outweighed the reduction.

### Wavy L4 Banded

| Metric | Value |
| --- | ---: |
| exact_build_time_ms | 999.691 |
| hierarchical_build_time_ms | 974.692 |
| speedup | 1.02565 |
| exact_bvh_sample_count | 1105240 |
| predicted_sample_count | 1127614 |
| quality_check_sample_count | 32656 |
| local_fallback_node_count | 962370 |
| near_surface_local_exact_node_count | 962370 |
| near_surface_predicted_node_count | 563390 |
| local_prediction_acceptance_rate | 0.369252 |
| max_abs_error | 0 |
| rms_error | 0 |
| p95_error | 0 |
| sign_mismatch_count | 0 |
| near_surface_sign_mismatch_count | 0 |
| quality_gate_passed | true |
| effective_speedup_claim_allowed | true |

This local benchmark demonstrates effective acceleration under selected
settings. The result is fixture- and parameter-specific and should not be
presented as a broad public speedup claim yet.

## Sign Query Count

The exact hotpath microbenchmark reports `sign_query_count = 14913536`.
Diagnostics-on hierarchical cases report sign queries through the diagnostic
counters:

- alpha.2-equivalent cube L5: `245280` diagnostic sign queries plus exact BVH
  sampling dominated by near-surface exact blocks.
- cube L5 banded: `8481600` sign queries.
- wavy L4 banded: `1105240` sign queries.

The optimized exact no-diagnostics CSV reports `sign_query_count = 0` because
the detailed counters are disabled on that fast path; this is not a claim that
sign queries were eliminated.

The new `--far-field-sign-policy exact|reuse-coarse|constant` option is parsed,
reported, and guarded away from near-surface blocks. It is not yet a complete
sign-query reduction implementation for the core far-field path.

## Benchmark Matrix Summary

| Case | Speedup | Exact BVH samples | Predicted samples | Local fallback nodes | Quality gate |
| --- | ---: | ---: | ---: | ---: | --- |
| cube L5 alpha.2 equivalent | 0.466316 | 11570720 | 3588096 | 0 | pass |
| cube L5 optimized exact | 0.94956 | 11570720 | 3588096 | 0 | pass |
| cube L5 banded | 0.598514 | 8481600 | 7451416 | 7462120 | pass |
| wavy L4 banded | 1.02565 | 1105240 | 1127614 | 962370 | pass |

All measured cases had zero max/RMS/P95 error and zero sign mismatches against
the sampled exact reference.

## Bottleneck

The immediate bottleneck is still the exact BVH near-surface hot path. The
bulk no-diagnostics path removes a large part of the per-block overhead, but
the optimized hotpath is still `1.28531x` the reference per-sample cost.

For banded near-surface sampling, the bottleneck shifts to the overhead needed
to make prediction safe: coarse sampling, local exact fallback masks, sparse
quality checks, and exact sampling of halo/near-band nodes.

## Recommendation

Do not promote this local sprint directly into a formal public version yet.
The results are promising but mixed:

- The exact hotpath overhead is greatly reduced but still above the 1.10x
  target.
- One selected wavy L4 banded case achieves `speedup > 1` with quality PASS.
- The cube L5 banded case still slows down.

Recommended next work:

- Continue local fallback tuning and avoid oversampling halo/near-band nodes.
- Add a normal-error audit before using banded prediction in normal-producing
  pipelines.
- Run larger and more varied model benchmarks before making a public claim.
- Complete an actual far-field sign reuse implementation if sign-query
  reduction remains a target.
