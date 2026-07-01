# DenseSDF Builder v1.5 Report

## Goal

AdaSDF-CL v1.5.0-alpha introduces a public standalone uniform DenseSDF builder
and an experimental adaptive builder interface preview.

## New Files

- `include/adasdf/geometry/DenseSDFModel.h`
- `include/adasdf/mesh/TriangleDistance.h`
- `include/adasdf/mesh/MeshSign.h`
- `include/adasdf/generation/DenseSDFBuilder.h`
- `include/adasdf/generation/DenseSDFBuildReportWriter.h`
- `include/adasdf/generation/AdaptiveSDFBuilderPreview.h`
- `include/adasdf/io/DenseSDFBin.h`
- `tools/adasdf_build_dense_sdf.cpp`
- `tools/adasdf_build_adaptive_sdf_preview.cpp`
- DenseSDF/adaptive examples and tests.

## DenseSDFModel Design

`DenseSDFModel` stores a uniform grid with origin, spacing, resolution, signed
flag, unit, and double-precision phi values. It supports trilinear
signed-distance sampling, finite-difference gradients/normals, AABB metadata,
memory footprint reporting, and direct query through the existing `SDFModel`
interface.

## DenseSDFBuilder Design

`DenseSDFBuilder` reads `TriangleMesh` or STL input, optionally runs mesh
diagnostics/readiness and safe cleanup, pads the mesh AABB, samples a uniform
grid, and computes nearest-triangle distances by brute force. Signed mode uses
`MeshSign` on watertight meshes. Unsigned mode stores positive distances.

## DenseSDFBin Format

The new `ADASDF_DENSE_SDFBIN_V1` format is a text format with explicit unit,
origin, spacing, resolution, signed flag, and values. The writer uses
round-trip-safe double precision and the reader validates required fields.

## CLI

`adasdf_build_dense_sdf` builds dense `.sdfbin` files and can write Markdown and
JSON-like reports. It validates output by reloading the file. Signed open-mesh
builds fail clearly; unsigned open-mesh builds are supported.

`adasdf_build_adaptive_sdf_preview` emits a dry-run adaptive plan and refuses
non-dry-run builds because full adaptive compressed construction is not
implemented in v1.5.0-alpha.

## Examples

- `examples/14_stl_to_dense_sdf_demo.cpp`
- `examples/15_adaptive_builder_preview_demo.cpp`

## Tests

Added coverage includes triangle distance, sign classification, dense builder,
dense `.sdfbin` roundtrip, dense CLI, dense collision, adaptive preview, and
adaptive preview CLI tests.

## Validation Results

- CPU-only configure/build: PASS.
- CPU-only CTest: 73/73 PASS.
- Install validation: PASS, including installed DenseSDF build/info/query/collide
  and adaptive preview dry-run.
- Alpha validation with install validation: PASS.
- Clean repository check: PASS.
- CUDA configure/build through an ASCII-only `subst` path: PASS.
- CUDA CTest: 73/73 PASS.

Direct CUDA configure from the non-ASCII source/build path fails in MSBuild
CUDA compiler identification because the generated object path is mangled by
the CUDA/MSBuild toolchain. The validated CUDA path maps the same workspace to
an ASCII drive letter before configuring.

## Current Limits

- Uniform dense grid only.
- Brute-force triangle loop; no BVH.
- Alpha-level ray-casting sign estimator.
- No hole filling or self-intersection repair.
- No adaptive octree/block construction.
- No low-rank compression.
- No GPU-native compressed SDF query.

## Next Steps

- v1.6 adaptive octree/block builder.
- v1.7 low-rank compression.
- v1.8 surrogate recommendation.
