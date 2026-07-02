# Implemented vs Planned

## Implemented Now

- Core-free demo SDF workflow.
- Demo surrogate recommender.
- Demo adaptive `.sdfbin`.
- Point signed-distance, gradient, and normal query.
- FCL-style pair collision API.
- Contact point, normal, penetration depth, and signed distance.
- Contact reduction and max contact cap.
- SVG collision visualization.
- CPU/CUDA query modes over direct or expanded layouts.
- Global and block expansion.
- Block selection.
- Benchmark timing semantics with warmup, repeat, kernel-only, and total-time
  fields.
- ExpandedSDF quality audit.
- Sign mismatch, ambiguous sign, and near-surface mismatch metrics.
- ASCII and binary STL reader for diagnostics.
- STL mesh diagnostics preflight for AABB, scale, watertight, boundary,
  non-manifold, duplicate, degenerate, component, and isolated-vertex checks.
- Mesh readiness scoring, severity classification, and repair suggestions for
  SDF build preflight.
- Safe mesh cleanup for near-duplicate vertices, degenerate triangles,
  duplicate triangles, and unused vertices.
- ASCII STL writer for cleaned mesh export.
- Standalone uniform DenseSDF builder for core-free STL-to-SDF construction.
- DenseSDF `.sdfbin` read/write with `ADASDF_DENSE_SDFBIN_V1`.
- `adasdf_build_dense_sdf` CLI and public STL-to-DenseSDF workflow.
- Adaptive octree builder for deterministic near-surface refinement.
- Adaptive block partitioner with one dense block per leaf.
- Adaptive block SDF builder for core-free STL-to-adaptive-block-SDF construction.
- Adaptive block `.sdfbin` read/write with `ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1`.
- `adasdf_build_adaptive_sdf` CLI and public STL-to-adaptive-block-SDF workflow.
- Small matrix-SVD utility for core-free block compression.
- `CompressedSDFBlock`, `CompressedAdaptiveBlockSDF`, and
  `CompressedAdaptiveBlockSDFModel`.
- Block low-rank compression with fixed-rank and error-bounded rank selection.
- Dense fallback blocks when compression cannot satisfy error targets.
- Compressed block `.sdfbin` read/write with `ADASDF_COMPRESSED_BLOCK_SDFBIN_V1`.
- `adasdf_compress_adaptive_sdf` and `adasdf_build_compressed_sdf` CLIs.
- Compression quality audit for dense adaptive vs compressed models.
- Surrogate-guided build recommendation with `adasdf_recommend_build`.
- `MeshFeatureExtractor`, `BuildSurrogateEstimator`, `SurrogateProfile`,
  `BuildRecommender`, and `BuildRecommendationWriter`.
- Markdown and JSON-like build recommendation reports.
- Copyable DenseSDF, AdaptiveBlockSDF, and CompressedAdaptiveBlockSDF commands
  from target error, memory budget, and use case.
- Pure-Python `adasdf_cli` CLI wrapper for installed AdaSDF-CL tools.
- Python dataclass result types, dry-run command previews, best-effort stdout
  parsers, and unittest coverage for the CLI wrapper.
- Sparse point-to-SDF collision query with `CollisionSampleSet`.
- Collision sample CSV I/O.
- Sparse collision-only, clearance, and candidate-search modes.
- Sample-radius proxy collision through `effective_phi = phi - radius`.
- Collision-only sparse early exit.
- Top-K contact candidate reduction with optional `reduction_radius`.
- `adasdf_sparse_query`, `adasdf_sparse_collide`,
  `adasdf_contact_candidates`, and `adasdf_benchmark_sparse_query`.
- Python wrappers for sparse query, sparse collision, contact candidates, and
  sparse benchmarking.
- Contact-aware active block selection for adaptive/compressed block SDFs.
- CPU active expanded-block cache for local compressed SDF runtime queries.
- `adasdf_select_active_blocks`, `adasdf_active_block_query`, and
  `adasdf_benchmark_block_cache`.
- Python wrappers for active block selection, active block query, and block
  cache benchmarking.
- Markdown and JSON-like mesh diagnostics reports.
- Before/after cleanup diagnostics and readiness reports.
- `adasdf_mesh_check` CLI, `adasdf_mesh_clean` CLI, CPU-only mesh diagnostics
  demo, and CPU-only mesh cleanup demo.
- External CMake install/export and downstream example.

## Partial / Experimental

- Surrogate recommender: deterministic demo recommender, not universal or fully
  trained.
- Adaptive builder: core-free adaptive octree/block dense builder is public.
- Compression builder: matrix-SVD block compression is implemented, while
  memory-bounded rank allocation remains a simple alpha-level heuristic.
- Build recommender: deterministic and heuristic-calibrated; not a universal
  trained model, not fully trained, and not an optimality guarantee.
- Existing-core bridge: available only when the existing research core and
  dependencies are present.
- CUDA expanded query: optional and experimental; no compressed-direct query.
- Block-expanded query: useful for selected/local contact regions.
- Contact manifold: contact reduction exists; stable robot-grade clustering is
  planned.
- Sparse contact candidates: deterministic Top-K candidates are implemented,
  but they are not full solver contacts and not a complete contact manifold.
- Real asset bridge: existing-core fixtures and sampled expansion bridge exist
  when direct query support is available.
- STL import audit: diagnostics, readiness suggestions, and safe cleanup are
  implemented, but no hole filling or self-intersection repair follows from
  the report yet.
- Adaptive compressed builder: v1.7 implements matrix-SVD block compression.
  Tucker/HOSVD and GPU-native compressed query remain planned.
- Python package distribution: v1.8.1 supports source-tree `PYTHONPATH` usage
  and editable install, but no pip release is performed.
- Direct compressed sparse query: useful for sparse queries, debugging,
  fallback, and small point sets, but not the main high-throughput GPU path.
- Active block cache: implemented on CPU in v1.10. CUDA active block residency
  remains planned.

## Planned

- CUDA active block expansion/cache for runtime memory savings.
- Tucker/HOSVD compression.
- Trained surrogate model integration.
- Online recommendation calibration.
- Complex mesh repair and hole filling.
- Self-intersection detection.
- FCL fallback backend.
- CollisionWorld broadphase.
- CCD.
- Solver-aware full contact manifold generation.
- Native Python / pybind11 bindings.
- C API.
- ROS / MoveIt integration.
- Real robot STL benchmark suite.
- Full low-rank GPU-native SDF query.
