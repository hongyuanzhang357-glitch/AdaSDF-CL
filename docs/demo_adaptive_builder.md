# Demo Adaptive Builder

AdaSDF-CL v0.9.0-alpha adds a core-free demo adaptive builder.

## Boundary

The v0.9 demo adaptive builder uses analytic box SDF queries and demo adaptive metadata to exercise the full public workflow. It is not the full adaptive compressed STL-to-SDF builder.

It does not load arbitrary STL meshes, train a surrogate model, or claim industrial parameter quality.

## Three Metadata Layers

`DemoAdaptiveSDFModel` stores:

- adaptive octree metadata through `DemoOctreeNodeInfo`;
- adaptive block layout through `DemoBlockInfo`;
- adaptive rank selection through each block's `rank`.

The builder is deterministic. Smaller `target_near_surface_error` increases selected resolution or rank. Smaller memory limits reduce block count and rank.

## Format

The writer emits:

```text
ADASDF_DEMO_ADAPTIVE_SDFBIN_V1
shape box
center 0 0 0
half_extent 0.5 0.5 0.5
unit m
target_near_surface_error 0.001
memory_limit_mb 64
block_expand_limit_mb 16
octree_nodes ...
blocks ...
surrogate demo_v0_9
warning demo_surrogate_not_universal_not_fully_trained
```

`SDFBinReader::read()` recognizes this format before falling back to v0.8 demo or existing-core readers.

## CLI

```bash
adasdf_build_demo_adaptive cube_adaptive.sdfbin --shape box --target-error 1e-3 --memory-mb 64 --block-memory-mb 16 --use-surrogate
```
