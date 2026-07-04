# Hierarchical Sampling Performance Diagnostics

## Purpose

The v1.16 hierarchical sampler is quality-first. It keeps near-surface blocks
exact, predicts only transition/far-field blocks, and falls back to exact
sampling when a quality check fails.

v1.16.0-alpha.2 adds diagnostics to explain whether the hierarchical path is
actually reducing exact work and where time is spent.

## Key Metrics

| Metric | Meaning |
| --- | --- |
| `near_surface_block_count` | Blocks kept exact by default. |
| `transition_block_count` | Blocks eligible for guarded transition prediction. |
| `far_field_block_count` | Blocks eligible for far-field interpolation. |
| `exact_bvh_sample_count` | Exact SDF samples issued by the hierarchical path. |
| `fine_sample_count` | Dense exact reference sample count. |
| `exact_sample_reduction_ratio` | `1 - exact_bvh_sample_count / fine_sample_count`. |
| `quality_check_sample_count` | Exact samples used by the quality guard. |
| `prediction_acceptance_rate` | Accepted predicted blocks divided by predicted blocks. |
| `fallback_rate` | Exact fallback blocks divided by predicted blocks. |
| `classification_time_ms` | Coarse classification and policy time. |
| `coarse_sampling_time_ms` | Exact BVH time spent on coarse probes. |
| `prediction_time_ms` | Fine-grid interpolation time. |
| `quality_check_time_ms` | Exact quality guard time. |
| `fallback_exact_time_ms` | Exact resampling time after guard failure. |
| `exact_sampling_time_ms` | Exact fine sampling time for exact blocks and fallback blocks. |

## alpha.2 Overhead Fixes

- Near-surface exact blocks now skip coarse probes.
- Non-near blocks generate one coarse sample grid and reuse it for
  classification and prediction.
- Far-field blocks default to corner quality checks.
- Transition quality checks have a separate sample count.
- Benchmark reports expose both quality and speedup claim gating.

## Claim Rule

A run may only be described as effective hierarchical acceleration when:

- `quality_gate_passed=true`;
- `speedup > 1`;
- `effective_speedup_claim_allowed=true`.

If quality passes but `speedup <= 1`, report it as a quality-preserving
diagnostic result, not as a speedup.

## Current Bottleneck

The alpha.2 smoke benchmarks show reduced exact sample counts but still do not
cross `speedup > 1`. The dominant remaining cost is exact BVH sampling for
near-surface blocks, especially on cube-like fixtures where most adaptive
leaves are classified as near-surface.
