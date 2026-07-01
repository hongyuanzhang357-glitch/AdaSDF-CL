# Adaptive Builder Interface Preview

AdaSDF-CL v1.8.0-alpha keeps the adaptive builder preview as a planning tool.
Octree refinement, block partitioning, dense block sampling, and matrix-SVD
low-rank block compression are implemented. v1.8 adds deterministic
surrogate-guided build recommendation. The preview documents the remaining
Tucker/HOSVD, trained-surrogate, and GPU-native compressed-query stages.

It does not generate `.sdfbin` files. Use `adasdf_build_adaptive_sdf` for real
block-wise dense adaptive SDF construction or `adasdf_build_compressed_sdf` for
one-step compressed adaptive block SDF output. Use `adasdf_recommend_build` for
parameter recommendation.

## CLI

```bash
adasdf_build_adaptive_sdf_preview model.stl model_adaptive.sdfbin --target-error 1e-3 --memory-mb 512 --dry-run --plan adaptive_plan.md
```

Dry-run mode returns success and writes the plan when `--plan` is supplied. If
the user requests a real build without `--dry-run`, the CLI fails clearly:

```text
Use adasdf_build_adaptive_sdf for block-wise dense adaptive SDF construction.
Use adasdf_build_compressed_sdf for one-step compressed output.
Use adasdf_recommend_build for parameter recommendation.
```

## Planned Stages

- Mesh diagnostics.
- Optional cleanup.
- Octree refinement: implemented in v1.6.
- Block partition: implemented in v1.6.
- Dense sampling: implemented in v1.6.
- Low-rank compression: implemented in v1.7 using matrix-SVD.
- Surrogate recommendation: implemented in v1.8 as a deterministic estimator.
- Tucker/HOSVD compression: planned.
- Trained surrogate model: planned.
- GPU-native compressed query: planned.
- Error audit.
- SDFBin write.

## Public Contract

`AdaptiveSDFBuilderPreview::makePlan()` returns a plan with
`implemented_in_this_version == true` for the implemented v1.6 octree/block
dense builder, v1.7 matrix-SVD compression scope, and v1.8 deterministic
recommendation scope.

The preview is useful for stabilizing option names, docs, and future workflow
shape. It is not a substitute for the implemented `adasdf_build_adaptive_sdf`
or `adasdf_build_compressed_sdf` builders, and it does not pretend that
Tucker/HOSVD, trained surrogate models, or GPU-native compressed query are
complete. The v1.8 recommender is not a universal trained model, not fully
trained, and not an optimality guarantee.

## Future Work

- v1.8: deterministic surrogate-guided recommendation.
- Future: trained surrogate model integration.
- v1.9: GPU-native adaptive/compressed query.
