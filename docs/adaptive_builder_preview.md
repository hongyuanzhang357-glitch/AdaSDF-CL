# Adaptive Builder Interface Preview

AdaSDF-CL v1.7.0-alpha keeps the adaptive builder preview as a planning tool.
Octree refinement, block partitioning, dense block sampling, and matrix-SVD
low-rank block compression are implemented. The preview documents the remaining
Tucker/HOSVD, surrogate, and GPU-native compressed-query stages.

It does not generate `.sdfbin` files. Use `adasdf_build_adaptive_sdf` for real
block-wise dense adaptive SDF construction or `adasdf_build_compressed_sdf` for
one-step compressed adaptive block SDF output.

## CLI

```bash
adasdf_build_adaptive_sdf_preview model.stl model_adaptive.sdfbin --target-error 1e-3 --memory-mb 512 --dry-run --plan adaptive_plan.md
```

Dry-run mode returns success and writes the plan when `--plan` is supplied. If
the user requests a real build without `--dry-run`, the CLI fails clearly:

```text
Use adasdf_build_adaptive_sdf for block-wise dense adaptive SDF construction.
Use adasdf_build_compressed_sdf for one-step compressed output.
```

## Planned Stages

- Mesh diagnostics.
- Optional cleanup.
- Octree refinement: implemented in v1.6.
- Block partition: implemented in v1.6.
- Dense sampling: implemented in v1.6.
- Low-rank compression: implemented in v1.7 using matrix-SVD.
- Tucker/HOSVD compression: planned.
- Surrogate recommendation: planned for v1.8.
- GPU-native compressed query: planned.
- Error audit.
- SDFBin write.

## Public Contract

`AdaptiveSDFBuilderPreview::makePlan()` returns a plan with
`implemented_in_this_version == true` for the implemented v1.6 octree/block
dense builder and v1.7 matrix-SVD compression scope.

The preview is useful for stabilizing option names, docs, and future workflow
shape. It is not a substitute for the implemented `adasdf_build_adaptive_sdf`
or `adasdf_build_compressed_sdf` builders, and it does not pretend that
Tucker/HOSVD, surrogate-guided recommendation, or GPU-native compressed query
are complete.

## Future Work

- v1.8: surrogate-guided recommendation.
- v1.9: GPU-native adaptive/compressed query.
