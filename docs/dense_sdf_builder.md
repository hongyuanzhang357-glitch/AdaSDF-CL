# DenseSDF Builder

AdaSDF-CL v1.5.0-alpha introduces a public standalone uniform DenseSDF
builder. This is the first core-free STL-to-SDF construction path in the public
repository.

The builder produces a uniform dense grid, not an adaptive octree, block
layout, low-rank compressed representation, or GPU-native compressed SDF.

v1.6.0-alpha adds a separate AdaptiveBlockSDF builder. v1.7.0-alpha adds a
separate CompressedAdaptiveBlockSDF workflow with matrix-SVD factors and dense
fallback. v1.8.0-alpha adds `adasdf_recommend_build` to recommend whether a
mesh should start with DenseSDF, AdaptiveBlockSDF, or
CompressedAdaptiveBlockSDF. DenseSDF remains the uniform baseline.
v1.12.0-alpha adds optional CPU BVH-accelerated sampling through
`--accel bvh`; `--accel brute` remains the default reference path.

## Workflow

```bash
adasdf_mesh_check model.stl --readiness --out mesh_report.md
adasdf_mesh_clean model.stl model_clean.stl --report cleanup_report.md
adasdf_recommend_build model_clean.stl --target-error 1e-3 --memory-mb 512 --use-case balanced --out recommendation.md --emit-command
adasdf_build_dense_sdf model_clean.stl model_dense.sdfbin --resolution 64 --padding 0.05 --accel bvh --threads 4
adasdf_info model_dense.sdfbin
adasdf_query model_dense.sdfbin --point 0 0 0
adasdf_collide model_dense.sdfbin model_dense.sdfbin --max-contacts 4
```

## Implemented Components

- `DenseSDFModel`: uniform grid model with direct signed-distance query,
  finite-difference gradient/normal query, AABB, metadata, and memory footprint.
- `TriangleDistance`: robust point-to-triangle distance with degenerate
  triangle fallback.
- `MeshSign`: deterministic ray-casting sign estimate for watertight meshes.
- `DenseSDFBuilder`: brute-force or optional BVH STL/TriangleMesh sampling
  into a uniform grid.
- `TriangleBVH` and `BVHSDFSampler`: optional CPU builder acceleration.
- `DenseSDFBuildReportWriter`: Markdown and JSON-like build reports.
- `DenseSDFBin`: text `.sdfbin` format with magic `ADASDF_DENSE_SDFBIN_V1`.
- `adasdf_build_dense_sdf`: public CLI for building dense `.sdfbin` assets.

## Signed And Unsigned Modes

Signed builds require a watertight mesh by default. Open meshes fail clearly in
signed mode. Users can request an unsigned distance field with `--unsigned`.

The v1.5 sign estimator is alpha-level ray casting. It is deterministic and
tested on project fixtures, but it is not industrial geometry certification and
does not prove that an arbitrary STL is free from self-intersections.

## Acceleration

`adasdf_build_dense_sdf` supports:

```bash
--accel brute
--accel bvh
--threads 4
--benchmark-brute-reference
```

BVH acceleration affects build-time sampling only. It is not GPU BVH and does
not change the DenseSDF query backend or file format.

## DenseSDFBin Format

The v1.5 dense text format starts with:

```text
ADASDF_DENSE_SDFBIN_V1
unit m
origin x y z
spacing dx dy dz
resolution nx ny nz
signed 1
values
phi0
phi1
...
```

The writer uses round-trip-safe double precision for origin, spacing, and
values.

## Current Limits

- Brute-force triangle loop, no BVH.
- Uniform dense grid only.
- No adaptive octree or block construction in this builder; use
  `adasdf_build_adaptive_sdf` for the v1.6 adaptive block dense builder.
- No low-rank compression in this uniform builder; use
  `adasdf_build_compressed_sdf` for the v1.7 compressed adaptive block path.
- Build recommendation is available separately through `adasdf_recommend_build`
  in v1.8.0-alpha.
- No hole filling or self-intersection repair.
- No generated `.sdfbin` assets are committed to the repository.

## Recommended Use

Use DenseSDF as a public, auditable baseline builder for small and moderate
test meshes, tutorials, CI validation, and comparisons against future adaptive
builders. For large production meshes, expect memory and build time to scale
with `resolution^3 * triangle_count`.
