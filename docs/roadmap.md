# Roadmap

## Completed Alpha Milestones

- v0.1 interface preview.
- v0.2 existing-core `.sdfbin` load/query bridge.
- v0.3 pair query API validation.
- v0.4 adaptive builder bridge.
- v0.5 CPU narrow-phase and contact reduction.
- v0.6 alpha release hardening.
- v0.7 install/package and external integration.
- v0.8 core-free standalone analytic demo backend.
- v0.9 surrogate-guided adaptive demo workflow and SVG contact visualization.
- v1.0 optional CUDA batch query backend and CPU/GPU benchmark tooling.
- v1.0.1 query mode, expansion mode, and GPU-resident expanded SDF query backend.
- v1.0.2 CUDA benchmark timing semantics and resident query workspace.
- v1.0.2-alpha.1 CI portability hotfix for Linux benchmark CLI tests.
- v1.0.3 CUDA benchmark output modes, no-download timing, workspace reporting,
  and block lookup fast-path refinement.
- v1.1 ExpandedSDF accuracy audit, sign mismatch metrics, near-surface risk
  reporting, and sampled existing-core expansion bridge.
- v1.1.1 capability exposure, public positioning, capability CLI, and
  walkthrough example.
- v1.2 STL mesh diagnostics, import audit, Markdown/JSON reports, and
  project-generated STL diagnostic fixtures.
- v1.3 mesh build readiness scoring, severity classification, and repair
  suggestions.
- v1.4 safe mesh cleanup, clean ASCII STL export, before/after cleanup reports,
  and standalone cleanup CLI.
- v1.5 standalone uniform DenseSDF builder, DenseSDF `.sdfbin` read/write,
  public STL-to-SDF workflow, and adaptive builder interface preview.
- v1.6 adaptive octree/block SDF builder, adaptive block dense `.sdfbin`
  read/write, public STL-to-adaptive-block-SDF workflow, and adaptive-vs-dense
  comparison.
- v1.7 matrix-SVD low-rank block compression, compressed adaptive block
  `.sdfbin` read/write, compression quality audit, and public
  STL-to-compressed-SDF workflow.
- v1.8 surrogate-guided build recommendation, deterministic build surrogate
  estimator, recommendation reports, and public recommend-then-build workflow.
- v1.8.1 pure-Python CLI wrapper, dataclass command results, dry-run command
  preview, best-effort parsers, and unittest validation.
- v1.9 sparse point-to-SDF collision, collision-only early exit, sample-radius
  proxy collision, Top-K contact candidate reduction, sparse benchmark CLI, and
  Python sparse wrappers.
- v1.10 contact-aware active block selection, CPU local block expansion cache,
  active block query CLI, block cache benchmark CLI, and Python active block
  wrappers.

## v1.10.0-alpha Scope

- `ActiveBlockSelector` for sparse contact-aware block id selection;
- CPU `ExpandedBlockCache` for local dense block residency;
- `BlockExpansionManager` for adaptive/compressed block expansion;
- `ActiveBlockQuery` with cache/fallback source reporting;
- `adasdf_select_active_blocks`, `adasdf_active_block_query`, and
  `adasdf_benchmark_block_cache`;
- Python wrapper helpers for active block selection, active block query, and
  block cache benchmarking;
- explicit boundary that CUDA active block cache and GPU-native compressed
  query remain planned.

## v1.9.0-alpha Scope

- `CollisionSampleSet` and CSV I/O;
- sparse query defaults to phi-only and computes normals only when requested;
- sample-radius proxy collision through `effective_phi = phi - radius`;
- collision-only, clearance, and candidate-search sparse modes;
- collision-only early exit for boolean collision checks;
- `adasdf_sparse_collide` return code `10` for detected collision, not failure;
- `ContactCandidateReducer` with deterministic Top-K selection and optional
  `reduction_radius`;
- `adasdf_sparse_query`, `adasdf_sparse_collide`,
  `adasdf_contact_candidates`, and `adasdf_benchmark_sparse_query`;
- Python wrapper helpers for sparse query, sparse collision, contact candidates,
  and sparse benchmarking;
- explicit boundary that sparse contact candidates are not solver contacts and
  not a complete contact manifold;
- explicit boundary that direct compressed query is suitable for sparse query,
  debugging, fallback, and small point sets, while high-throughput runtime
  memory saving is provided by v1.10 CPU active block expansion/cache.

## v1.8.1-alpha Scope

- `python/adasdf_cli` pure-Python package;
- subprocess runner over installed AdaSDF-CL CLI tools;
- `ADASDF_BIN`, `ADASDF_CL_BIN`, `PATH`, and `bin_dir` tool discovery;
- helpers for mesh check, mesh cleanup, build recommendation, DenseSDF build,
  AdaptiveBlockSDF build, compressed SDF build, info, query, collide,
  expansion quality, and benchmark;
- dataclass result types that preserve command, stdout, stderr, return code,
  and elapsed time;
- dry-run command preview;
- best-effort stdout parsers;
- unittest coverage and validation-script integration;
- explicit boundary that this is not pybind11, not a native Python binding, and
  not a C++ extension module.

## v1.8.0-alpha Scope

- `adasdf_recommend_build` CLI;
- `MeshFeatureExtractor` over STL diagnostics/readiness features;
- `BuildSurrogateEstimator` for deterministic memory/error/compression/cost
  estimates;
- `SurrogateProfile` key-value calibration constants;
- `BuildRecommender` and `BuildRecommendationWriter`;
- DenseSDF, AdaptiveBlockSDF, and CompressedAdaptiveBlockSDF candidate
  recommendation;
- target error, memory budget, use case, signed/unsigned policy, dense fallback
  policy, and compression preference inputs;
- Markdown and JSON-like recommendation reports;
- copyable build command output;
- explicit boundary that the recommender is experimental, deterministic, not a
  universal trained model, not fully trained, and not an optimality guarantee.

## v1.7.0-alpha Scope

- `SmallMatrixSVD` for baseline small block compression without Eigen/LAPACK;
- `CompressedSDFBlock` and `CompressedAdaptiveBlockSDF`;
- `BlockLowRankCompressor` with fixed-rank and error-bounded rank selection;
- dense fallback blocks when error targets are not satisfied;
- `CompressedAdaptiveBlockSDFModel` with direct CPU query and finite-difference
  gradients;
- `ADASDF_COMPRESSED_BLOCK_SDFBIN_V1` read/write support;
- `adasdf_compress_adaptive_sdf` and `adasdf_build_compressed_sdf`;
- compression quality reports and DenseSDF/AdaptiveBlockSDF/compressed
  comparison examples;
- compressed SDF support in `adasdf_info`, `adasdf_query`, `adasdf_collide`,
  expansion quality, and batch benchmark `--model`;
- explicit boundary that Tucker/HOSVD, trained recommendation models, and
  GPU-native compressed query remain planned.

## v1.6.0-alpha Scope

- standalone `AdaptiveOctree` and deterministic near-surface refinement;
- `AdaptiveBlockPartitioner` with one dense block per leaf;
- `AdaptiveBlockSDFBuilder` for STL / `TriangleMesh` input;
- signed watertight builds and unsigned open-mesh builds;
- `AdaptiveBlockSDFModel` with direct query and finite-difference gradient;
- `ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1` read/write support;
- `adasdf_build_adaptive_sdf` CLI with Markdown/JSON reports, dry-run, and
  reload validation;
- adaptive block support in `adasdf_info`, `adasdf_query`, `adasdf_collide`,
  expansion quality, and batch benchmark `--model`;
- adaptive vs dense comparison example and tests;
- adaptive builder preview updated to mark octree/block construction as
  implemented and low-rank compression as planned;
- explicit boundary that block data remains dense and low-rank compression is
  planned for v1.7.0-alpha.

## v1.5.0-alpha Scope

- standalone `DenseSDFModel` with trilinear query and finite-difference
  gradients;
- point-triangle distance utilities and alpha-level ray-casting sign estimate;
- brute-force STL-to-uniform-DenseSDF builder;
- signed watertight builds and unsigned open-mesh builds;
- `ADASDF_DENSE_SDFBIN_V1` read/write support;
- `adasdf_build_dense_sdf` CLI with report output and reload validation;
- DenseSDF support in `adasdf_info`, `adasdf_query`, `adasdf_collide`,
  expansion quality, and batch benchmark `--model`;
- adaptive builder interface preview with options, stages, dry-run plan, and
  explicit non-implementation boundary;
- dense/adaptive preview examples and tests;
- explicit `QueryModeConfig` for backend, expansion, block selection, resident
  data, and fallback;
- reusable `ExpandedSDF`, `SDFExpander`, and `QueryEngine` APIs;
- CPU direct, CPU global-expanded, and CPU block-expanded query paths;
- CUDA global-expanded and CUDA block-expanded query paths using resident GPU
  data;
- deterministic benchmark CLI with backend, expansion, block, memory, setup,
  timing breakdown, warmup/repeat, kernel-only, phi-only, total-time, and error
  columns;
- reusable CUDA query workspace for benchmark repeats over resident expanded
  SDF data;
- CUDA phi-only expanded SDF kernel for signed-distance-only kernel comparison;
- CPU/CUDA query-mode tests and graceful CUDA-unavailable skip behavior;
- documentation of the original UI CUDA dense/block-expanded reference and the
  v1.0.2 timing-comparison boundary;
- explicit benchmark `output=phi` and `output=phi,normal` modes;
- CUDA device-only no-download benchmark mode;
- reusable output-buffer path for resident CUDA benchmark repeats;
- block neighbor fallback that avoids rescanning the already-tested center
  block;
- benchmark workspace and block lookup CSV fields;
- expanded-grid resolution, padding, near-surface band, and sign-epsilon
  quality controls;
- deterministic `ExpansionQuality` direct-vs-expanded audit reports;
- sign mismatch, ambiguous sign, near-surface sign mismatch, fallback count,
  and fallback-rate metrics;
- `adasdf_expansion_quality` CLI and query-mode demo quality audit option;
- sampled existing-core `.sdfbin` to `ExpandedSDF` bridge when direct query is
  available, with graceful skip when the existing core is unavailable;
- capability matrix and implemented/partial/planned docs;
- FCL complement strategy and public positioning docs;
- query backend and contact output matrices;
- `adasdf_capabilities` CLI;
- CPU-only capability walkthrough example;
- `TriangleMesh`, `STLReader`, `STLWriter`, `MeshDiagnostics`,
  `MeshReadiness`, `MeshCleanup`, and `MeshDiagnosticsWriter`;
- ASCII and binary STL reader for diagnostics;
- topology-level watertight, boundary-edge, non-manifold-edge, duplicate,
  degenerate, component, isolated-vertex, AABB, and scale checks;
- `adasdf_mesh_check` CLI with Markdown and JSON-like report output;
- `MeshReadiness` scoring from diagnostics to SDF build suitability;
- severity classification for info, warning, and critical mesh issues;
- repair and preprocessing suggestions without modifying input STL files;
- `adasdf_mesh_check --readiness` with strict and lenient modes;
- `MeshCleanup` safe cleanup for near-duplicate vertices, degenerate triangles,
  duplicate triangles, and unused vertices;
- ASCII `STLWriter` for cleaned mesh export;
- `adasdf_mesh_clean` standalone cleanup CLI;
- `adasdf_mesh_check --clean-out` and `--clean-report` before/after workflow;
- CPU-only mesh diagnostics and mesh cleanup examples;
- project-generated STL diagnostics and cleanup fixtures.

The original `v1.0.2-alpha`, `v1.0.2-alpha.1`, `v1.0.3-alpha`,
`v1.1.0-alpha`, `v1.1.1-alpha`, `v1.2.0-alpha`, `v1.3.0-alpha`,
`v1.4.0-alpha`, `v1.5.0-alpha`, `v1.6.0-alpha`, `v1.7.0-alpha`,
`v1.8.0-alpha`, and `v1.8.1-alpha` tags are
retained for traceability. The recommended public pre-release is
`v1.9.0-alpha`.

## Future Work

- CUDA active block expansion/cache after v1.10;
- Tucker/HOSVD compression;
- trained surrogate model integration and online recommendation calibration;
- optional hole-filling workflows with explicit user control;
- self-intersection detection and repair workflows;
- real trained surrogate integration with public artifacts and validation
  cards;
- native Python / pybind11 bindings and C API;
- full low-rank compressed SDF GPU expansion and compressed-direct CUDA query;
- CUDA-accelerated pair collision pipeline;
- optional real FCL adapter;
- stricter distance and contact quality benchmarks.
