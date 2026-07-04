# AdaSDF-CL v1.16 Speed-Quality Summary

## Scope

This summary compares the v1.16 hierarchical adaptive sampling smoke
benchmarks. The numbers are fixture-level measurements, not universal
performance claims.

Effective acceleration is only claimed when both conditions are true:

- `speedup > 1`;
- `quality_gate_passed=true`.

## alpha.1 Baseline

| Case | exact_build_time_ms | hierarchical_build_time_ms | speedup | quality_gate_passed |
| --- | ---: | ---: | ---: | --- |
| cube L4 | 908.183 | 1535.82 | 0.591 | true |
| cube L5 | 6850.62 | 21061.0 | 0.325 | true |

alpha.1 preserved quality but did not demonstrate construction-speed
acceleration.

## alpha.2 Setup

- Acceleration: TriangleBVH
- Block resolution: 8
- Coarse resolution: 3 for fixed cases
- Transition quality-check samples: 2
- Far-field quality-check mode: `corners`
- Far-field safety factor: 2.0
- Threads: 2
- Diagnostics: enabled for benchmark runs

## alpha.2 Fixed Cases

| Case | exact_build_time_ms | hierarchical_build_time_ms | speedup | exact_sample_reduction_ratio | prediction_acceptance_rate | fallback_rate | quality_gate_passed | effective_speedup_claim_allowed |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |
| cube L4 | 984.452 | 1506.14 | 0.654 | 0.105 | 1.000 | 0.000 | true | false |
| cube L5 | 7234.26 | 10476.2 | 0.691 | 0.224 | 1.000 | 0.000 | true | false |
| wavy sphere L4 | 958.770 | 1289.72 | 0.743 | 0.252 | 1.000 | 0.000 | true | false |

All three alpha.2 fixed cases pass the quality gate with zero sampled
comparison error and zero sign mismatches. None satisfy `speedup > 1`.

## alpha.2 Sweep

The wavy sphere sweep tested 36 combinations over:

- coarse resolution: 2, 3, 4;
- transition quality-check samples: 1, 2, 3;
- far-field quality-check modes: `corners`, `sparse`;
- far-field safety factor: 2.0, 3.0.

Best quality-pass result:

| Case | coarse_resolution | transition_quality_check_samples | far_field_quality_check | far_field_safety_factor | speedup | exact_sample_reduction_ratio | prediction_acceptance_rate | fallback_rate |
| --- | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: |
| sweep_3 | 2 | 1 | sparse | 3.0 | 0.823 | 0.262 | 1.000 | 0.000 |

No sweep case satisfied `speedup > 1 && quality_gate_passed=true`.

## Interpretation

v1.16.0-alpha.2 reduces obvious overhead compared with alpha.1:

- near-surface exact blocks skip the coarse probe path;
- non-near blocks reuse coarse samples for classification and prediction;
- far-field blocks use relaxed quality checks by default;
- diagnostics expose block classes, exact BVH sample counts, prediction
  acceptance, fallback, and timing buckets.

The bottleneck is still exact BVH sampling for near-surface blocks. The cube
fixtures are dominated by near-surface exact blocks, and the synthetic fixture
still has enough exact work that hierarchical overhead remains larger than the
saved exact samples.

## Claim Boundary

AdaSDF-CL v1.16.0-alpha.2 preserves quality and improves diagnostics and
overhead behavior, but it does not yet demonstrate effective construction-speed
acceleration on the measured smoke fixtures.
