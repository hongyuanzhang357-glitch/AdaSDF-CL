# Hierarchical Adaptive Sampling

AdaSDF-CL v1.16.0-alpha adds an experimental, opt-in hierarchical sampling path
for adaptive block SDF construction.

## Purpose

The goal is to reduce exact point-to-triangle or TriangleBVH sampling work
during SDF construction while preserving near-surface quality. This is a
construction-time optimization. It does not change `.sdfbin` file formats or
runtime query semantics.

## Classification

Each adaptive leaf block is classified as:

- near-surface;
- transition;
- far-field.

Near-surface blocks are sampled exactly by default. Transition and far-field
blocks may use coarse-to-fine prediction, but only when the quality guard
accepts the prediction.

## CLI

```bash
adasdf_build_adaptive_sdf model.stl model.sdfbin --sampling hierarchical
adasdf_build_compressed_sdf model.stl model.sdfbin --sampling hierarchical
```

Useful tuning options:

- `--coarse-resolution N`
- `--quality-check-samples N`
- `--target-sampling-error value`
- `--far-field-interpolation` / `--no-far-field-interpolation`
- `--transition-prediction` / `--no-transition-prediction`
- `--quality-guard` / `--no-quality-guard`

## Boundaries

The default remains exact sampling. `--no-quality-guard` does not accept unsafe
predictions; the policy falls back to exact sampling decisions. This feature is
not a GPU builder, not Tucker/HOSVD compression, and not a collision solver
change.
