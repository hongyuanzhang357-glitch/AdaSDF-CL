# AdaSDF-CL v1.6 Adaptive Block SDF Builder Report

## Goal

AdaSDF-CL v1.6.0-alpha implements the first public core-free adaptive
octree/block SDF builder on top of the v1.5 DenseSDF baseline:

```text
STL -> diagnostics/readiness -> optional cleanup -> adaptive octree
    -> adaptive dense blocks -> ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1
    -> info/query/collide/benchmark/expansion-quality tools
```

This release implements adaptive octree refinement and block-wise dense SDF
storage. It does not implement low-rank compression, SVD/Tucker compression,
surrogate-guided parameter recommendation, or GPU-native compressed SDF query.

## Added Files

- `include/adasdf/generation/AdaptiveOctree.h`
- `include/adasdf/generation/AdaptiveOctreeBuilder.h`
- `include/adasdf/generation/AdaptiveBlock.h`
- `include/adasdf/generation/AdaptiveBlockPartitioner.h`
- `include/adasdf/generation/AdaptiveBlockSDFBuilder.h`
- `include/adasdf/generation/AdaptiveBlockSDFBuildReportWriter.h`
- `include/adasdf/geometry/AdaptiveBlockSDFModel.h`
- `include/adasdf/io/AdaptiveBlockSDFBin.h`
- Matching `src/` implementations.
- `tools/adasdf_build_adaptive_sdf.cpp`
- `examples/16_stl_to_adaptive_block_sdf_demo.cpp`
- `examples/17_adaptive_vs_dense_sdf_demo.cpp`
- Adaptive octree/block/model/bin/CLI/collision/consistency tests.
- v1.6 workflow, format, comparison, release draft, and report docs.

## Modified Files

- Version metadata: `include/adasdf/version.h`,
  `cmake/adasdf_cl_version.cmake`, `CITATION.cff`.
- Build/install wiring: `CMakeLists.txt`, umbrella header, install targets.
- SDF I/O dispatch: `SDFBinReader`, `SDFBinWriter`, `adasdf_info`.
- Capability and tool docs: README, CHANGELOG, roadmap, matrices, examples and
  tools README files.
- Validation scripts: install and alpha validation now exercise the adaptive
  build/info/query/collide/dry-run paths.
- Adaptive builder preview wording: octree/block dense construction is now
  implemented; low-rank remains planned.

## AdaptiveOctree Design

`AdaptiveOctreeBuilder` creates a deterministic root AABB from the mesh bounds
plus padding. Each node stores level, bounds, sampled phi statistics, sign-change
state, near-surface status, and child ids. Leaves cover the full root domain;
near-surface status is metadata, not a filter.

Refinement rules include:

- mandatory refinement below `min_level`;
- optional sign-change refinement;
- sampled near-surface band refinement;
- target near-surface error refinement;
- hard stop at `max_level`.

Distance sampling uses the core-free triangle distance implementation. Signed
mode uses `MeshSign`.

## AdaptiveBlock Design

`AdaptiveBlockPartitioner` maps each octree leaf to one dense block. Each block
stores stable ids, source octree node id, level, bounds, origin, spacing, cubic
resolution, signed/unsigned mode, near-surface flag, and dense phi values.

The default v1.6 block storage is dense per-block phi data. There is no rank,
factor matrix, SVD, Tucker, or compressed payload in this format.

## AdaptiveBlockSDFBuilder Design

`AdaptiveBlockSDFBuilder` can build from `TriangleMesh` or STL. The STL path
runs diagnostics and readiness checks, optionally applies safe cleanup, builds
the octree, partitions leaves into blocks, samples each block grid, and returns
an `AdaptiveBlockSDFModel`.

The v1.6 sampler is deliberately simple and audit-friendly: brute-force
point-triangle distance, optional sign assignment, deterministic block order, and
small default test resolutions. BVH acceleration is planned work.

## Signed And Unsigned Modes

Signed mode is available for watertight meshes and requires watertight input by
default. Open meshes fail in signed mode unless the caller explicitly chooses an
unsigned build path. Unsigned mode stores non-negative distances and is intended
for open mesh diagnostics and early workflow checks.

## Watertight Requirement

The default signed STL workflow checks mesh readiness before sampling. If the
mesh is not watertight, the build fails instead of silently producing unreliable
signed distances. `--unsigned` / `--allow-open-unsigned` enables open-mesh
unsigned output.

## SDFBin Format

`ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1` is a human-auditable text format with global
bounds, signed mode, block count, block resolution, per-block bounds, origin,
spacing, level, near-surface metadata, and dense phi values.

`SDFBinReader::read()` loads the new format as an `AdaptiveBlockSDFModel`.
`SDFBinWriter::write()` writes the format when given an adaptive block model.

## CLI

`adasdf_build_adaptive_sdf` supports:

- target error and surface-band factor;
- min/max octree levels;
- block resolution and padding;
- signed and unsigned mode;
- watertight requirement control;
- safe cleanup;
- Markdown and JSON-like reports;
- dry-run planning;
- reload validation.

Passing `--enable-low-rank` returns an unsupported-feature status and states
that low-rank compression is planned for v1.7.0-alpha.

## Dense vs Adaptive

DenseSDF remains the uniform-grid baseline introduced in v1.5. AdaptiveBlockSDF
uses octree leaves and per-block dense grids introduced in v1.6. v1.6 does not
promise lower memory usage for every mesh; it establishes the public adaptive
block workflow and serializable format before compression.

## Examples

- `16_stl_to_adaptive_block_sdf_demo`: generated cube STL, adaptive build,
  reload, query, collision, and block summary.
- `17_adaptive_vs_dense_sdf_demo`: builds dense and adaptive SDFs for the same
  generated cube and compares sample phi values and memory footprints.

Both examples write generated files to build temp locations, not the source tree.

## Tool Compatibility

Adaptive block `.sdfbin` files are supported by:

- `adasdf_info`
- `adasdf_query`
- `adasdf_collide`
- `adasdf_expansion_quality`
- `adasdf_benchmark_batch_query --model`

Global expansion works through the generic `SDFModel` interface. Block-native
expanded query remains a future optimization.

## Validation Results

- CPU configure/build: PASS.
- CPU CTest: 82/82 PASS.
- CLI smoke: adaptive build/info/query/collide/dry-run/quality/benchmark PASS.
- Install validation: PASS.
- Alpha validation: PASS.
- Clean repo check: PASS.
- CUDA configure/build via ASCII `subst` path: PASS, CUDA 12.6 detected.
- CUDA CTest: 82/82 PASS.

Representative CLI smoke:

```text
adasdf_build_adaptive_sdf closed_cube_ascii.stl cube_adaptive_block.sdfbin
  --target-error 1e-3 --max-level 4 --block-resolution 8 --padding 0.05

Format: ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1
Leaf blocks: 4040
Near-surface blocks: 3584
Reload validation: success
Low-rank compression: not enabled / planned for v1.7.0-alpha
```

## Current Limits

- Brute-force triangle distance sampling.
- Linear adaptive block lookup in the direct model.
- No BVH or spatial acceleration structure.
- No low-rank/SVD/Tucker compression.
- No surrogate recommendation for adaptive build parameters.
- No GPU-native adaptive/compressed query.
- Block-native expander integration is partial; global expansion is supported.

## Next Steps

- v1.7: low-rank block compression.
- v1.8: surrogate-guided recommendation.
- v1.9: GPU-native adaptive/compressed query.
