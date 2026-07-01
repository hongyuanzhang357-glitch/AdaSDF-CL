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
`v1.1.0-alpha`, `v1.1.1-alpha`, `v1.2.0-alpha`, `v1.3.0-alpha`, and
`v1.4.0-alpha` tags are retained for traceability. The recommended public
pre-release is `v1.5.0-alpha`.

## Future Work

- adaptive octree/block STL-to-SDF builder;
- low-rank compression builder;
- surrogate-guided builder recommendation;
- optional hole-filling workflows with explicit user control;
- self-intersection detection and repair workflows;
- real trained surrogate integration with public artifacts and validation
  cards;
- Python package;
- full low-rank compressed SDF GPU expansion and compressed-direct CUDA query;
- CUDA-accelerated pair collision pipeline;
- optional real FCL adapter;
- stricter distance and contact quality benchmarks.
