# Alpha Status

AdaSDF-CL 1.7.0-alpha is a research-preview release candidate.

The original `v1.0.2-alpha`, `v1.0.2-alpha.1`, `v1.0.3-alpha`,
`v1.1.0-alpha`, `v1.1.1-alpha`, `v1.2.0-alpha`, `v1.3.0-alpha`,
`v1.4.0-alpha`, `v1.5.0-alpha`, and `v1.6.0-alpha` tags are retained for
traceability. The recommended public pre-release is `v1.7.0-alpha`.

## What Works

- Core-free analytic box SDF backend.
- Demo surrogate recommender.
- Demo adaptive builder with octree, block, and rank metadata.
- Demo adaptive `.sdfbin` read/write with `ADASDF_DEMO_ADAPTIVE_SDFBIN_V1`.
- Point query and collision query over demo adaptive models.
- `CollisionRequest::max_contacts` and CLI `--max-contacts` enforcement.
- SVG collision visualization.
- Installable CMake package with downstream no-input demo adaptive path.
- CPU batch query for signed distance, gradient, and normal data.
- Query-mode configuration for backend, expansion mode, block selection,
  resident expanded data, and CPU fallback policy.
- CPU direct, CPU global-expanded, CPU block-expanded, CUDA global-expanded,
  and CUDA block-expanded `QueryEngine` paths.
- Optional CUDA resident expanded SDF backend for core-free analytic/demo
  adaptive boxes.
- CUDA query workspace reuse for benchmark repeats over resident expanded SDF data.
- CUDA phi-only expanded SDF kernel for signed-distance-only kernel timing.
- Explicit benchmark output modes: `output=phi` and `output=phi,normal`.
- CUDA device-only benchmark mode for no-download upper-bound timing.
- ExpandedSDF quality audit against direct SDF queries.
- Sign mismatch, ambiguous sign, near-surface sign mismatch, and fallback-rate
  metrics.
- `adasdf_expansion_quality` CLI with CSV output.
- `adasdf_capabilities` CLI and capability walkthrough example.
- ASCII and binary STL reader for diagnostics.
- ASCII STL writer for cleaned mesh export.
- `TriangleMesh`, `STLReader`, `STLWriter`, `MeshDiagnostics`,
  `MeshReadiness`, `MeshCleanup`, and `MeshDiagnosticsWriter`.
- `adasdf_mesh_check` CLI with diagnostics, readiness scoring, safe cleanup
  hooks, Markdown, and JSON-like reports.
- `adasdf_mesh_clean` CLI for safe cleanup and clean ASCII STL output.
- `DenseSDFModel`, `DenseSDFBuilder`, `DenseSDFBin`, and
  `adasdf_build_dense_sdf` for a public core-free STL-to-uniform-SDF path.
- DenseSDF query, collision, expansion quality, and benchmark model loading.
- `AdaptiveOctree`, `AdaptiveOctreeBuilder`, `AdaptiveBlockPartitioner`,
  `AdaptiveBlockSDFBuilder`, `AdaptiveBlockSDFModel`, and
  `AdaptiveBlockSDFBin` for a public core-free STL-to-adaptive-block-SDF path.
- `adasdf_build_adaptive_sdf` CLI, adaptive block build reports, and
  `ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1`.
- AdaptiveBlockSDF query, collision, expansion quality, and benchmark model
  loading.
- `SmallMatrixSVD`, `CompressedSDFBlock`, `BlockLowRankCompressor`,
  `CompressedAdaptiveBlockSDFModel`, and `CompressedBlockSDFBin` for the public
  core-free matrix-SVD compressed adaptive block SDF path.
- `adasdf_compress_adaptive_sdf` and `adasdf_build_compressed_sdf`.
- `ADASDF_COMPRESSED_BLOCK_SDFBIN_V1`.
- Compression quality reports with max/mean/RMS/p95 error and sign mismatch
  metrics.
- Compressed SDF query, collision, expansion quality, and benchmark model
  loading.
- `AdaptiveSDFBuilderPreview` and `adasdf_build_adaptive_sdf_preview` dry-run
  planning for low-rank compressed construction.
- Project-generated STL diagnostics and cleanup fixtures.
- Capability, query backend, contact output, FCL complement, mesh cleanup, and
  public positioning documentation.
- Deterministic 10k/100k/1M benchmark CLI with CSV output, memory columns,
  setup/kernel/total timings, detailed timing breakdowns, warmup/repeat
  statistics, workspace reuse fields, block lookup fields, fallback counts,
  quality metrics, and error columns.
- Existing-core bridge remains available when configured separately.

## Current Boundary

The v0.9 demo surrogate is not universal, not fully trained, and not an
optimality guarantee. It is a deterministic public workflow exerciser.

The demo adaptive builder uses analytic box SDF queries and demo metadata. It
is not the full adaptive compressed STL-to-SDF builder. v1.6 adds a real
adaptive octree/block dense STL builder, but low-rank compression remains
planned.

The v1.1.x CUDA path is optional and intentionally narrow. It supports batch
queries only after SDF data has been pre-expanded into global or block dense
data. `CUDA + None` is rejected because compressed-direct GPU query is not
implemented. This is not a full low-rank compressed SDF GPU expansion and is
not an industrial GPU collision pipeline.

ExpandedSDF is an approximation of direct model queries. v1.1.x adds quality
metrics so users can inspect absolute error, p95 error, sign mismatch,
near-surface sign mismatch, and fallback behavior before using an expanded
layout in contact workflows.

v1.1.1-alpha is a capability exposure release. It does not add a FCL backend,
ROS / MoveIt integration, CCD, complex mesh repair, or new core algorithms.

v1.2.0-alpha is an STL mesh diagnostics release. It reads STL files and reports
preflight topology/scale issues. It does not repair meshes, detect
self-intersections, or build arbitrary-STL adaptive SDF assets.

v1.3.0-alpha is a mesh readiness release. It turns diagnostics into severity
classification, a 0-100 SDF build readiness score, and repair suggestions. It
does not modify input STL files and is not an automatic mesh repair engine.

v1.4.0-alpha is a safe mesh cleanup release. It can merge near-duplicate
vertices, remove degenerate triangles, remove duplicate triangles, remove
unused vertices, and export a cleaned ASCII STL. It does not fill holes, repair
self-intersections, boolean reconstruct a surface, infer units, change model
scale, or replace an industrial CAD repair tool.

v1.5.0-alpha is a standalone uniform DenseSDF builder release. The DenseSDF
builder is implemented and core-free. It uses a brute-force triangle loop and
uniform grid sampling.

v1.6.0-alpha is an adaptive octree/block dense SDF builder release. The builder
is implemented and core-free. It uses brute-force triangle sampling and stores
dense phi values per adaptive block.

v1.7.0-alpha is a matrix-SVD low-rank block compression release. It can compress
adaptive dense blocks, keep dense fallback blocks when targets are not met, and
query compressed models directly on CPU. Tucker/HOSVD compression,
surrogate-guided recommendation, and GPU-native compressed query remain
planned.

Benchmark `total_ms` is a full query timing. Benchmark `kernel_ms` is CUDA
kernel event timing. Original UI warmed kernel-average numbers should be
compared to `--kernel-only --output phi --reuse-resident` rows.

`output=phi` computes signed distance only. `output=phi,normal` computes signed
distance plus finite-difference normals. `--device-only` skips result download
and correctness checks and should be used only for GPU-side performance
analysis.

## Validation Snapshot

- Expected tests: 93.
- Expected install validation: PASS with `ADASDF_CL_USE_EXISTING_CORE=OFF`.
- Expected alpha validation: PASS.
- Expected clean check: PASS.
- Target external collision test verdict for v0.9.0-alpha: PASS for the demo
  adaptive workflow.
- Expected CUDA-unavailable behavior: GPU benchmark/tests SKIPPED, not FAILED.
- Current v1.7.0-alpha local CPU CTest result: 93/93 PASS.
- Current v1.7.0-alpha local CUDA CTest result: 93/93 PASS via ASCII `subst`
  path with CUDA 12.6.
- Current v1.7.0-alpha install validation: PASS.
- Current v1.7.0-alpha alpha validation: PASS with install validation.
- Current v1.7.0-alpha clean check: PASS.
