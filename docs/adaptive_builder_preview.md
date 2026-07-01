# Adaptive Builder Interface Preview

AdaSDF-CL v1.6.0-alpha keeps the adaptive builder preview as a planning tool.
Octree refinement, block partitioning, and dense block sampling are now
implemented by `adasdf_build_adaptive_sdf`. The preview documents the remaining
low-rank and surrogate stages.

It does not generate `.sdfbin` files. Use `adasdf_build_adaptive_sdf` for real
block-wise dense adaptive SDF construction.

## CLI

```bash
adasdf_build_adaptive_sdf_preview model.stl model_adaptive.sdfbin --target-error 1e-3 --memory-mb 512 --dry-run --plan adaptive_plan.md
```

Dry-run mode returns success and writes the plan when `--plan` is supplied. If
the user requests a real build without `--dry-run`, the CLI fails clearly:

```text
Use adasdf_build_adaptive_sdf for block-wise dense adaptive SDF construction.
Low-rank compression is planned for v1.7.0-alpha.
```

## Planned Stages

- Mesh diagnostics.
- Optional cleanup.
- Octree refinement: implemented in v1.6.
- Block partition: implemented in v1.6.
- Dense sampling: implemented in v1.6.
- Low-rank compression: planned for v1.7.
- Error audit.
- SDFBin write.

## Public Contract

`AdaptiveSDFBuilderPreview::makePlan()` returns a plan with
`implemented_in_this_version == true` for the v1.6 octree/block dense builder
scope.

The preview is useful for stabilizing option names, docs, and future workflow
shape. It is not a substitute for the implemented `adasdf_build_adaptive_sdf`
builder and does not pretend that low-rank compressed construction is complete.

## Future Work

- v1.7: low-rank compression.
- v1.8: surrogate-guided recommendation.
