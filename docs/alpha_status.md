# Alpha Status

AdaSDF-CL 1.11.0-alpha is a research-preview release candidate.

The original `v1.0.2-alpha`, `v1.0.2-alpha.1`, `v1.0.3-alpha`,
`v1.1.0-alpha`, `v1.1.1-alpha`, `v1.2.0-alpha`, `v1.3.0-alpha`,
`v1.4.0-alpha`, `v1.5.0-alpha`, `v1.6.0-alpha`, `v1.7.0-alpha`,
`v1.8.0-alpha`, `v1.8.1-alpha`, `v1.9.0-alpha`, and `v1.10.0-alpha` tags are
retained for traceability. The recommended public pre-release is
`v1.11.0-alpha`.

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
- `MeshFeatureExtractor`, `BuildSurrogateEstimator`, `SurrogateProfile`,
  `BuildRecommender`, and `BuildRecommendationWriter` for deterministic build
  recommendation.
- `adasdf_recommend_build` for recommending DenseSDF, AdaptiveBlockSDF, or
  CompressedAdaptiveBlockSDF commands from STL, target error, memory budget,
  and use case.
- Markdown and JSON-like build recommendation reports with candidate tables,
  confidence, warnings, rationale, and copyable CLI commands.
- Pure-Python `adasdf_cli` wrapper for installed command-line tools.
- Python wrapper helpers for mesh check, cleanup, recommendation, SDF builds,
  info, query, collide, expansion quality, benchmark, and small workflows.
- Python wrapper dataclass results, dry-run previews, best-effort parsers, and
  unittest coverage.
- `CollisionSampleSet` and `CollisionSampleSetIO` for sparse point samples.
- `SparseSDFQuery` for phi-only or phi+normal sparse point-to-SDF query.
- `SparseCollisionQuery` for collision-only, clearance, and candidate-search
  modes.
- Sample radius support through `effective_phi = phi - radius`.
- Collision-only early-exit sparse query.
- `ContactCandidateReducer` for deterministic Top-K candidate selection and
  optional `reduction_radius`.
- `adasdf_sparse_query`, `adasdf_sparse_collide`, `adasdf_contact_candidates`,
  and `adasdf_benchmark_sparse_query`.
- Python wrapper helpers for sparse query, sparse collision, contact
  candidates, and sparse benchmarking.
- `ActiveBlockSelector` for deterministic contact-aware active block id
  selection from sparse samples or explicit block ids.
- `ActiveExpandedBlock`, `ExpandedBlockCache`, and `BlockExpansionManager` for
  CPU local block expansion and deterministic LRU cache residency.
- `ActiveBlockQuery` for querying selected expanded local blocks with
  cache/fallback source reporting.
- `adasdf_select_active_blocks`, `adasdf_active_block_query`, and
  `adasdf_benchmark_block_cache`.
- Python wrapper helpers for active block selection, active block query, and
  block cache benchmarking.
- `CudaActiveBlockBuffer`, `CudaActiveBlockCache`, `CudaActiveBlockQuery`, and
  `CudaActiveBlockBenchmark` for optional CUDA local active-block query over
  CPU-expanded active dense blocks.
- `adasdf_cuda_active_block_query` and
  `adasdf_benchmark_cuda_block_cache`.
- Python wrapper helpers for CUDA active block query and CUDA block cache
  benchmarking.
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

The v1.11 CUDA active block path is also optional and intentionally bounded. It
selects active blocks on CPU, expands them on CPU, uploads the expanded local
dense blocks to GPU, and runs local dense-grid interpolation on CUDA. It is not
GPU-native compressed SVD reconstruction and does not reconstruct low-rank
factors per point on GPU.

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
query compressed models directly on CPU. Tucker/HOSVD compression and
GPU-native compressed query remain planned.

v1.8.0-alpha is a surrogate-guided build recommendation release. It recommends
DenseSDF, AdaptiveBlockSDF, or CompressedAdaptiveBlockSDF parameters and emits
a build command. It is deterministic and heuristic-calibrated, not a universal
trained model, not fully trained, and not an optimality guarantee. It does not
run the build by default.

v1.8.1-alpha is a Python CLI wrapper release. It provides a pure-Python
subprocess wrapper over installed AdaSDF-CL CLI tools. It is not pybind11, not a
native Python binding, and not a C++ extension module. Native Python bindings
remain planned.

v1.9.0-alpha is a sparse SDF collision release. Sparse query defaults to
phi-only and does not compute normals unless requested. Collision-only sparse
query supports early exit and `adasdf_sparse_collide` return code `10` means
collision detected, not failure. Contact candidates are Top-K reduced
candidates for contact budgets, not full solver contacts and not a complete
contact manifold. Direct compressed query is useful for sparse queries,
debugging, fallback, and small point sets, but not the main high-throughput GPU
path.

v1.10.0-alpha is a CPU active block cache release. It selects local adaptive or
compressed blocks near sparse/contact samples, expands only those blocks, and
reuses the dense local blocks through a deterministic CPU cache. This is the
intended runtime memory-saving path for compressed SDF contact workflows. It is
not CUDA active block residency, not GPU block upload, not GPU-native
compressed query, not FCL fallback, not `CollisionWorld`, and not a complete
contact solver. CUDA active block cache is planned for v1.11.

Benchmark `total_ms` is a full query timing. Benchmark `kernel_ms` is CUDA
kernel event timing. Original UI warmed kernel-average numbers should be
compared to `--kernel-only --output phi --reuse-resident` rows.

`output=phi` computes signed distance only. `output=phi,normal` computes signed
distance plus finite-difference normals. `--device-only` skips result download
and correctness checks and should be used only for GPU-side performance
analysis.

## Validation Snapshot

- Expected tests: 121.
- Expected Python wrapper unittest: PASS, with real CLI smoke enabled when
  `ADASDF_BIN`, `ADASDF_TEST_STL`, and `ADASDF_TEST_SAMPLES` are set.
- Expected install validation: PASS with `ADASDF_CL_USE_EXISTING_CORE=OFF`.
- Expected alpha validation: PASS.
- Expected clean check: PASS.
- Target external collision test verdict for v0.9.0-alpha: PASS for the demo
  adaptive workflow.
- Expected CUDA-unavailable behavior: GPU benchmark/tests SKIPPED, not FAILED.
- Current v1.10.0-alpha local CPU CTest target: 121/121 PASS.
- Current v1.10.0-alpha CUDA validation is optional and should skip gracefully
  when CUDA is unavailable.
- Current v1.10.0-alpha Python wrapper unittest target: 31/31 PASS.
- Current v1.10.0-alpha install validation target: PASS.
- Current v1.10.0-alpha alpha validation target: PASS with install validation.
- Current v1.10.0-alpha clean check target: PASS.
