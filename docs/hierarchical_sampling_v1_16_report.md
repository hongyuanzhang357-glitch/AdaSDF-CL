# AdaSDF-CL v1.16.0-alpha Hierarchical Sampling Report

## Goal

Add an experimental hierarchical adaptive sampling path for adaptive block SDF
construction that can reduce exact BVH sampling work while preserving a safe
default path.

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

## Expected Follow-up

- Tune classification thresholds on larger real meshes.
- Add richer sampled quality datasets before treating speedups as release
  performance claims.
- Explore parallel hierarchical block sampling after the single-threaded
  reference path is stable.
