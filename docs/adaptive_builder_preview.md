# Adaptive Builder Interface Preview

AdaSDF-CL v1.5.0-alpha adds an experimental adaptive builder interface preview.
It exposes planned options, stages, and dry-run reporting for future
octree/block/low-rank construction.

It does not generate adaptive compressed `.sdfbin` files in v1.5.0-alpha.

## CLI

```bash
adasdf_build_adaptive_sdf_preview model.stl model_adaptive.sdfbin --target-error 1e-3 --memory-mb 512 --dry-run --plan adaptive_plan.md
```

Dry-run mode returns success and writes the plan when `--plan` is supplied. If
the user requests a real build without `--dry-run`, the CLI fails clearly:

```text
Full adaptive compressed SDF build is not implemented in v1.5.0-alpha.
```

## Planned Stages

- Mesh diagnostics.
- Optional cleanup.
- Octree refinement.
- Block partition.
- Dense sampling.
- Low-rank compression.
- Error audit.
- SDFBin write.

## Public Contract

`AdaptiveSDFBuilderPreview::makePlan()` returns a plan with
`implemented_in_this_version == false`.

The preview is useful for stabilizing option names, docs, and future workflow
shape. It is not a substitute for the implemented DenseSDF builder and does not
pretend that adaptive compressed construction is complete.

## Future Work

- v1.6: adaptive octree/block builder.
- v1.7: low-rank compression.
- v1.8: surrogate-guided recommendation.
