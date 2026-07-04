# AdaSDF-CL v1.16.0-alpha.2 Hierarchical Sampling Report

## Goal

v1.16.0-alpha.2 diagnoses why v1.16 hierarchical adaptive sampling was slower
than exact sampling and reduces obvious overhead while preserving quality.

This release does not change `.sdfbin` formats and does not enter v1.17 scope.

## What Changed

- Added `HierarchicalSamplingDiagnostics`.
- Added default-off exact sampler counters.
- Skipped coarse probes for near-surface exact blocks.
- Reused coarse samples for classification and prediction in non-near blocks.
- Added far-field quality-check modes: `none`, `corners`, `sparse`, and `full`.
- Added transition-specific quality-check sample counts.
- Added diagnostics report/CSV outputs to the hierarchical benchmark.
- Added `adasdf_sweep_hierarchical_sampling`.
- Added `wavy_sphere_ascii.stl` as a small synthetic non-cube fixture.

## alpha.1 Baseline

| Case | exact_build_time_ms | hierarchical_build_time_ms | speedup | quality_gate_passed |
| --- | ---: | ---: | ---: | --- |
| cube L4 | 908.183 | 1535.82 | 0.591 | true |
| cube L5 | 6850.62 | 21061.0 | 0.325 | true |

alpha.1 preserved quality but did not reduce total construction time.

## alpha.2 Fixed Benchmark Results

| Case | exact_build_time_ms | hierarchical_build_time_ms | speedup | exact_bvh_sample_count | predicted_sample_count | quality_check_sample_count | fallback_rate | prediction_acceptance_rate | quality_gate_passed |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| cube L4 | 984.452 | 1506.14 | 0.654 | 1850968 | 233472 | 3648 | 0.000 | 1.000 | true |
| cube L5 | 7234.26 | 10476.2 | 0.691 | 11570720 | 3588096 | 56064 | 0.000 | 1.000 | true |
| wavy sphere L4 | 958.770 | 1289.72 | 0.743 | 1564330 | 564224 | 8816 | 0.000 | 1.000 | true |

## Block Classification

| Case | total_block_count | near_surface_block_count | transition_block_count | far_field_block_count |
| --- | ---: | ---: | ---: | ---: |
| cube L4 | 4040 | 3584 | 0 | 456 |
| cube L5 | 29128 | 22120 | 0 | 7008 |
| wavy sphere L4 | 4082 | 2980 | 0 | 1102 |

The smoke fixtures remain dominated by near-surface exact blocks. That limits
the maximum possible construction-speed gain from far-field prediction.

## Timing Diagnostics

| Case | classification_time_ms | coarse_sampling_time_ms | prediction_time_ms | quality_check_time_ms | fallback_exact_time_ms | exact_sampling_time_ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| cube L4 | 0.570 | 11.387 | 3.158 | 3.801 | 0.000 | 1409.35 |
| cube L5 | 5.963 | 181.235 | 55.944 | 57.848 | 0.000 | 9498.06 |
| wavy sphere L4 | 0.622 | 21.992 | 7.310 | 6.487 | 0.000 | 1154.71 |

Exact fine sampling remains the dominant cost.

## Sweep Result

The wavy sphere sweep tested 36 configurations. Best quality-pass result:

| Case | coarse_resolution | transition_quality_check_samples | far_field_quality_check | far_field_safety_factor | speedup | exact_sample_reduction_ratio | prediction_acceptance_rate | fallback_rate |
| --- | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| sweep_3 | 2 | 1 | sparse | 3.0 | 0.823 | 0.262 | 1.000 | 0.000 |

No sweep case satisfied `speedup > 1 && quality_gate_passed=true`.

## Conclusion

v1.16.0-alpha.2 improves the hierarchical path and makes the performance
failure mode explicit. Quality gates pass, exact samples are reduced, and
fallbacks are avoided in the measured cases. However, no measured case can
claim effective acceleration because all speedups remain below 1.

The next optimization step should target the near-surface exact-dominated
region. Candidate approaches include more conservative near/far partitioning,
surface-aware block splitting, cheaper exact near-surface sampling, or a
separate builder strategy for cases where almost all leaves are near-surface.
