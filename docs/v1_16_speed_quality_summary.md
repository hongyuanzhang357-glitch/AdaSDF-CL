# AdaSDF-CL v1.16 Speed-Quality Summary

## Scope

This summary answers whether the v1.16 hierarchical adaptive sampling path
shows construction-time speedup on the project fixture benchmark.

These numbers are fixture-level smoke benchmark results and should not be
presented as universal performance claims. Larger STL models are needed for
representative speedup evaluation.

这些数值只是项目自带 fixture 下的 smoke benchmark, 不能代表复杂工业 STL
的普适加速比例。复杂模型需要单独 benchmark。

## Benchmark Setup

- Fixture: `tests/data/mesh_diagnostics/closed_cube_ascii.stl`
- Builder backend: CPU-only
- Acceleration: TriangleBVH
- Block resolution: 8
- Coarse resolution: 3
- Quality check samples: 3
- Target error: `1e-3`
- Threads: 2

## Results

| Case | exact_build_time_ms | hierarchical_build_time_ms | speedup | speedup > 1 | max_abs_error | rms_error | p95_error | sign_mismatch_count | near_surface_sign_mismatch_count | predicted_sample_count | fallback_block_count | quality_gate_passed |
| --- | ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| cube L4 | 908.183 | 1535.82 | 0.591335 | false | 0 | 0 | 0 | 0 | 0 | 233472 | 8 | true |
| cube L5 | 6850.62 | 21061.0 | 0.325275 | false | 0 | 0 | 0 | 0 | 0 | 3588096 | 8 | true |

## Interpretation

Both smoke cases passed the quality gate with zero measured phi error and zero
sign mismatch on the sampled comparison points.

Neither case can be described as an effective speedup. In this small cube
fixture and current parameter setting, hierarchical sampling overhead, coarse
prediction, quality checks, and fallback accounting exceed the savings from
reduced exact sampling.

The current result is therefore:

- quality: passed for the smoke fixture;
- speed: no effective acceleration demonstrated on this fixture;
- claim boundary: do not present these fixture numbers as universal
  construction speedup.

## Next Measurement Step

Use larger, project-generated or clearly licensed STL fixtures with more
far-field volume and more varied block classifications. The hierarchical path
is most likely to show useful speedup when exact sampling dominates prediction
and quality-check overhead.
