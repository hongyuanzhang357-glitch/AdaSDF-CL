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
- Hierarchical adaptive block sampling for adaptive/compressed builders with
  near-surface exact default, guarded prediction, exact fallback, and
  `adasdf_benchmark_hierarchical_sampling`.
- Contact-focused narrow-band SDF construction for collision-oriented use
  cases with opt-in `--sampling contact-band`.
- `ContactBandMarker`, `ContactBandBlockSampler`, contact-band phi/sign/normal
  quality audit, coverage audit, marker cost audit, end-to-end timing fields,
  `adasdf_benchmark_contact_band_sampling`, and
  `adasdf_sweep_contact_band_sampling`.
- Compression quality audit for dense adaptive vs compressed models.
- CPU TriangleBVH acceleration for DenseSDF, AdaptiveBlockSDF, and
  CompressedAdaptiveBlockSDF builder sampling.
- Deterministic stdlib parallel builder sampling.
- `adasdf_benchmark_builder_acceleration` for builder acceleration reports.
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
- Solver-aware contact candidate stabilization with `ContactBudget`,
  `ContactPatch`, `ContactClusterer`, `ContactStabilizer`, `SolverContactSet`,
  and deterministic `stable_key` export.
- `adasdf_stabilize_contacts`, `adasdf_solver_contact_candidates`, and
  `adasdf_benchmark_contact_reduction`.
- `WorldTransform`, `CollisionGroupMask`, `WorldObject`, `CollisionWorld`,
  `AABBBroadphase`, `WorldSceneIO`, `WorldSparseCollision`,
  `WorldSolverContacts`, and `WorldReportWriter`.
- `adasdf_world_broadphase`, `adasdf_world_sparse_collide`,
  `adasdf_world_solver_contacts`, and `adasdf_benchmark_collision_world`.
- Python wrappers for world broadphase, world sparse collision, world
  solver-ready contacts, and CollisionWorld benchmarking.
- Strict JSON/CSV reproducibility reports, report validation, and run summary
  collection.
- `adasdf_write_manifest`, `adasdf_validate_report`, and
  `adasdf_collect_run_summary`.
- Contact-aware active block selection for adaptive/compressed block SDFs.
- CPU active expanded-block cache for local compressed SDF runtime queries.
- `adasdf_select_active_blocks`, `adasdf_active_block_query`, and
  `adasdf_benchmark_block_cache`.
- Python wrappers for active block selection, active block query, and block
  cache benchmarking.
- CUDA active block cache baseline over CPU-expanded active dense blocks.
- `adasdf_cuda_active_block_query` and
  `adasdf_benchmark_cuda_block_cache`.
- Python wrappers for CUDA active block query and CUDA block cache
  benchmarking.
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
- Sparse contact candidates: deterministic Top-K candidates are implemented.
  v1.13 can stabilize them into solver-ready candidates, but this is still not
  a full solver, not solver constraints, and not impulses or friction.
- CollisionWorld: deterministic AABB broadphase and sample-based SDF
  narrowphase are implemented. This is not exact mesh-vs-mesh contact, not FCL
  fallback, not CCD, not a scene graph, and not a physics solver.
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
- Active block cache: implemented on CPU in v1.10 and as a CUDA upload/query
  baseline in v1.11; persistent GPU residency reuse remains planned.
- CUDA active block query: CPU still performs selection and expansion; CUDA
  performs local dense-grid interpolation over uploaded active blocks.
- BVH builder acceleration: implemented on CPU for builder sampling only; no
  GPU BVH and no FCL fallback.
- Hierarchical sampling: experimental speed/quality path. The exact path
  remains default, prediction is never accepted without a quality guard, and
  `.sdfbin` storage remains unchanged.
- Contact-focused narrow-band sampling: alpha collision-oriented construction
  path. It protects the contact band and relaxes far-field phi; it should not
  be described as global full-field SDF reconstruction acceleration.

## Planned

- GPU BVH or other GPU-native builder acceleration.
- Persistent CUDA active block residency reuse and faster active block lookup.
- Tucker/HOSVD compression.
- Trained surrogate model integration.
- Online recommendation calibration.
- Complex mesh repair and hole filling.
- Self-intersection detection.
- FCL fallback backend.
- CCD.
- Temporal contact coherence and warm-start matching for solver contacts.
- Full contact solver, friction, Jacobians, and impulses.
- Native Python / pybind11 bindings.
- C API.
- ROS / MoveIt integration.
- Real robot STL benchmark suite.
- Full low-rank GPU-native SDF query.
