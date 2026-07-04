# AdaSDF-CL v1.16.0-alpha Hierarchical Sampling Report

## Goal

Add an experimental hierarchical adaptive sampling path for adaptive block SDF
construction that can reduce exact BVH sampling work while preserving a safe
default path.

This release focuses on construction-time sampling. It does not change the SDF
binary formats, the query runtime, collision-world semantics, or contact solver
behavior.

## Added Files

- `include/adasdf/sampling/BlockClassification.h`
- `include/adasdf/sampling/HierarchicalSamplingPolicy.h`
- `include/adasdf/sampling/HierarchicalSDFPredictor.h`
- `include/adasdf/sampling/SamplingQualityGuard.h`
- `include/adasdf/sampling/HierarchicalBlockSampler.h`
- `include/adasdf/sampling/SpeedQualityBenchmark.h`
- `include/adasdf/sampling/HierarchicalSamplingReportWriter.h`
- `src/sampling/BlockClassification.cpp`
- `src/sampling/HierarchicalSamplingPolicy.cpp`
- `src/sampling/HierarchicalSDFPredictor.cpp`
- `src/sampling/SamplingQualityGuard.cpp`
- `src/sampling/HierarchicalBlockSampler.cpp`
- `src/sampling/SpeedQualityBenchmark.cpp`
- `src/sampling/HierarchicalSamplingReportWriter.cpp`
- `tools/adasdf_benchmark_hierarchical_sampling.cpp`
- `tests/test_block_classification.cpp`
- `tests/test_hierarchical_sampling_policy.cpp`
- `tests/test_hierarchical_sdf_predictor.cpp`
- `tests/test_sampling_quality_guard.cpp`
- `tests/test_hierarchical_block_sampler.cpp`
- `tests/test_adaptive_builder_hierarchical_sampling.cpp`
- `tests/test_compressed_builder_hierarchical_sampling_cli.cpp`
- `tests/test_benchmark_hierarchical_sampling_cli.cpp`
- `tests/test_hierarchical_vs_exact_quality.cpp`
- `docs/github_release_draft_v1_16_0_alpha.md`

## Modified Files

- `CMakeLists.txt`
- `include/adasdf/adasdf.h`
- `include/adasdf/version.h`
- `include/adasdf/generation/AdaptiveBlockSDFBuilder.h`
- `src/generation/AdaptiveBlockSDFBuilder.cpp`
- `src/generation/AdaptiveBlockSDFBuildReportWriter.cpp`
- `tools/adasdf_build_adaptive_sdf.cpp`
- `tools/adasdf_build_compressed_sdf.cpp`
- `tools/adasdf_build_dense_sdf.cpp`
- `tools/adasdf_capabilities.cpp`
- `scripts/run_alpha_validation.py`
- `scripts/run_install_validation.py`
- `python/adasdf_cli/__init__.py`
- `python/adasdf_cli/config.py`
- `python/adasdf_cli/parsers.py`
- `python/adasdf_cli/tools.py`
- `python/pyproject.toml`
- `python/tests/test_parsers.py`
- `python/tests/test_tools_command_building.py`
- `README.md`
- `CHANGELOG.md`
- `docs/alpha_status.md`
- `docs/capability_matrix.md`
- `docs/implemented_vs_planned.md`
- `docs/python_cli_wrapper.md`
- `docs/roadmap.md`
- `python/README.md`
- `tools/README.md`

## Implemented

- `BlockClassifier` classifies coarse block probes as near-surface,
  transition, or far-field.
- `HierarchicalSamplingPolicy` maps each classification to exact sampling,
  guarded transition prediction, or guarded far-field interpolation.
- `HierarchicalSDFPredictor` performs trilinear coarse-to-fine prediction.
- `SamplingQualityGuard` checks deterministic exact samples before accepting a
  predicted block.
- `HierarchicalBlockSampler` falls back to exact sampling when prediction or
  quality validation fails.
- `SpeedQualityBenchmark` and `adasdf_benchmark_hierarchical_sampling` compare
  exact and hierarchical builds.

## Builder Integration

The default builder path remains exact:

```bash
adasdf_build_adaptive_sdf model.stl model.sdfbin
```

Hierarchical sampling is opt-in:

```bash
adasdf_build_adaptive_sdf model.stl model.sdfbin \
  --sampling hierarchical \
  --coarse-resolution 3 \
  --quality-check-samples 3 \
  --target-sampling-error 1e-3
```

The same sampling flags are accepted by `adasdf_build_compressed_sdf`.

## Safety Boundaries

- Near-surface blocks remain exact by default.
- Prediction is not accepted without the quality guard.
- Rejected predictions fall back to exact sampling.
- `--no-quality-guard` does not allow unsafe prediction; it forces exact
  sampling decisions in the hierarchical policy.
- `.sdfbin` formats are unchanged.
- This is a sampling-stage optimization, not GPU-native compressed query, not
  Tucker/HOSVD compression, and not a solver/contact algorithm change.

## Benchmark CLI

```bash
adasdf_benchmark_hierarchical_sampling model.stl \
  --max-level 2 \
  --block-resolution 5 \
  --coarse-resolution 2 \
  --quality-check-samples 2 \
  --comparison-samples 3 \
  --csv hier.csv \
  --report hier.md \
  --json hier.json
```

The benchmark reports exact build time, hierarchical build time, speedup,
max/mean/RMS/p95 error, sign mismatch counts, exact sample counts, predicted
sample counts, fallback block count, and quality gate status.

## Validation Coverage

- Unit tests for block classification, sampling policy, coarse prediction,
  quality guard, and block sampler.
- Builder integration test for hierarchical adaptive sampling.
- CLI tests for compressed builder hierarchical sampling and hierarchical
  benchmark output.
- Exact-vs-hierarchical quality test.
- Python wrapper dry-run and parser tests.
- Install and alpha validation coverage for
  `adasdf_benchmark_hierarchical_sampling`.

## Block Classification

`BlockClassifier` uses sign-change and absolute-distance checks on a coarse
block probe. The policy is deterministic and produces one of three block
importance classes: near-surface, transition, or far-field.

Near-surface classification is intentionally conservative because these blocks
are most likely to affect sign consistency and contact candidates.

## Hierarchical Sampling Policy

`HierarchicalSamplingPolicy` maps block importance to a sampling mode:

- Near-surface blocks use `ExactBVH`.
- Transition blocks may use `CoarsePredictThenCheck`.
- Far-field blocks may use `InterpolatedFarField`.

When hierarchical sampling is disabled, every block uses the exact path. When
the quality guard is disabled, the policy also falls back to exact decisions
instead of accepting unchecked predictions.

## Coarse-To-Fine Predictor

`HierarchicalSDFPredictor` expands coarse samples to a fine block grid with
trilinear interpolation. The predictor returns finite phi values and records
exact/predicted sample counts. Prediction is only an initial estimate and must
pass the quality guard before it can be accepted.

## Quality Guard

`SamplingQualityGuard` samples deterministic check points inside the block,
compares predicted phi against exact BVH sampling, and rejects predictions on
max, RMS, p95, sign, or near-surface sign mismatch failures.

## Fallback Exact

`HierarchicalBlockSampler` falls back to exact BVH sampling whenever
prediction fails, quality validation fails, or policy options require exact
sampling. Fallback counts are reported in the sampling stats and benchmark
metrics.

## Builder Integration

`AdaptiveBlockSDFBuilder` now accepts `HierarchicalSamplingOptions`. The default
builder path remains exact. `adasdf_build_adaptive_sdf` and
`adasdf_build_compressed_sdf` expose:

- `--sampling exact|hierarchical`
- `--coarse-resolution`
- `--quality-check-samples`
- `--far-field-interpolation` / `--no-far-field-interpolation`
- `--transition-prediction` / `--no-transition-prediction`
- `--near-surface-exact`
- `--quality-guard` / `--no-quality-guard`
- `--target-sampling-error`

`adasdf_build_dense_sdf` accepts the sampling option for CLI compatibility but
continues to use exact dense sampling in v1.16.

## Speed-Quality Metrics

`adasdf_benchmark_hierarchical_sampling` reports:

- exact build time;
- hierarchical build time;
- speedup;
- max, mean, RMS, and p95 phi error;
- sign mismatch counts;
- near-surface sign mismatch counts;
- exact sample counts;
- predicted sample counts;
- fallback block counts;
- quality gate status.

The speedup is informational; quality metrics determine whether a sampling
configuration is acceptable.

## Speed-Quality Benchmark Summary

v1.16.0-alpha.1 added fixture-level smoke benchmark measurements for
`closed_cube_ascii.stl`:

| Case | exact_build_time_ms | hierarchical_build_time_ms | speedup | max_abs_error | rms_error | p95_error | sign_mismatch_count | near_surface_sign_mismatch_count | predicted_sample_count | fallback_block_count | quality_gate_passed |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| cube L4 | 908.183 | 1535.82 | 0.591335 | 0 | 0 | 0 | 0 | 0 | 233472 | 8 | true |
| cube L5 | 6850.62 | 21061.0 | 0.325275 | 0 | 0 | 0 | 0 | 0 | 3588096 | 8 | true |

These numbers are fixture-level smoke benchmark results and should not be
presented as universal performance claims. Larger STL models are needed for
representative speedup evaluation.

The quality gate passed in both smoke cases, with zero measured phi error and
zero sign mismatch. However, neither case showed effective construction
speedup: hierarchical overhead exceeded exact-sampling savings for this simple
cube fixture and current parameters.

v1.16.0-alpha.2 adds diagnostics and overhead reductions. It improves over the
alpha.1 cube L5 result but still does not cross the effective speedup boundary:

| Case | exact_build_time_ms | hierarchical_build_time_ms | speedup | exact_sample_reduction_ratio | prediction_acceptance_rate | fallback_rate | quality_gate_passed | effective_speedup_claim_allowed |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |
| cube L4 | 984.452 | 1506.14 | 0.654 | 0.105 | 1.000 | 0.000 | true | false |
| cube L5 | 7234.26 | 10476.2 | 0.691 | 0.224 | 1.000 | 0.000 | true | false |
| wavy sphere L4 | 958.770 | 1289.72 | 0.743 | 0.252 | 1.000 | 0.000 | true | false |

The alpha.2 wavy sphere sweep tested 36 combinations. The best quality-pass
case reached speedup 0.823, so no measured case can be described as effective
construction-speed acceleration. The quality gates pass, but
`effective_speedup_claim_allowed` remains false because `speedup <= 1`.

## Validation Results

- CPU-only CMake configure/build: PASS.
- Full CPU CTest: PASS, 189/189 tests.
- Targeted v1.16 CTest subset: PASS, 9/9 tests.
- Python unittest: PASS, 54 tests, 1 skipped.
- Install validation: PASS.
- Alpha validation with install validation: PASS.
- `python scripts/check_repo_clean.py .`: PASS.
- `git diff --check`: PASS.
- Desktop root artifact scan for `adasdf_cl_build*`, `adasdf_cl_install`,
  `cube_*.sdfbin`, and `bench_*.csv`: PASS, no matches found.

All build, install, validation, generated, benchmark, report, log, and temp
artifacts used for this round were written under the user-provided `WORKROOT`
outside the repository.

No `.sdfbin`, `.csv`, `.svg`, `.log`, build, install, or validation artifacts
were committed.

## Current Limitations

- Hierarchical sampling is experimental and opt-in.
- Classification thresholds are conservative defaults, not tuned per mesh
  family.
- The implementation is CPU-only and does not add a GPU builder.
- This is block-local hierarchical sampling, not global minimum-cell tensor
  interpolation.
- It does not implement Tucker/HOSVD, GPU-native compressed query, FCL
  fallback, CCD, CollisionWorld extensions, or contact solver changes.

## Expected Follow-up

- Tune classification thresholds on larger real meshes.
- Add richer sampled quality datasets before treating speedups as release
  performance claims.
- Explore parallel hierarchical block sampling after the single-threaded
  reference path is stable.
- Target the near-surface exact-dominated bottleneck before claiming
  construction-speed acceleration.
