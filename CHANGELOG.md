# Changelog

## 1.7.0-alpha

Low-rank block compression for adaptive block SDFs.

### Added

- Small matrix-SVD utility for block compression.
- `CompressedSDFBlock` and `CompressedAdaptiveBlockSDF` data structures.
- `BlockLowRankCompressor` with fixed-rank and error-bounded rank selection.
- `CompressedAdaptiveBlockSDFModel`.
- `ADASDF_COMPRESSED_BLOCK_SDFBIN_V1` read/write support.
- Compression quality audit.
- `adasdf_compress_adaptive_sdf` CLI.
- `adasdf_build_compressed_sdf` CLI.
- Compressed SDF query, collision, benchmark and roundtrip tests.
- Dense vs adaptive vs compressed comparison examples.

### Changed

- Version updated to `1.7.0-alpha`.
- Adaptive builder preview now marks matrix-SVD low-rank block compression as
  implemented.
- `adasdf_build_adaptive_sdf --enable-low-rank` can emit compressed block SDFs.
- Capability matrix updated for the compressed SDF workflow.

### Notes

- Matrix-SVD block compression is implemented.
- Tucker/HOSVD compression is not implemented in v1.7.0-alpha.
- Surrogate-guided recommendation remains planned for v1.8.0-alpha.
- GPU-native compressed query remains planned; CUDA workflows use expanded SDF
  paths.

## 1.6.0-alpha

Adaptive octree/block SDF builder.

### Added

- Adaptive octree data structure and builder.
- Adaptive block partitioner.
- Adaptive block SDF model.
- Adaptive block SDF builder.
- `ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1` read/write support.
- `adasdf_build_adaptive_sdf` CLI.
- Adaptive block build reports.
- Adaptive vs dense SDF comparison example.
- Adaptive block SDF query, collision and roundtrip tests.

### Changed

- Version updated to `1.6.0-alpha`.
- Adaptive builder preview now marks octree/block construction as implemented
  and low-rank compression as planned.
- `adasdf_info`, `SDFBinReader`, `SDFBinWriter`, benchmark model loading, and
  expansion quality can use adaptive block `.sdfbin` files.

### Notes

- Block data is stored as dense per-block phi values.
- Low-rank compression is not implemented in v1.6.0-alpha.
- SVD/Tucker compression is planned for v1.7.0-alpha.
- Surrogate-guided recommendation is planned for v1.8.0-alpha.

## 1.5.0-alpha

Standalone DenseSDF builder and adaptive builder interface preview.

### Added

- Uniform `DenseSDFModel`.
- Brute-force STL-to-DenseSDF builder.
- Point-triangle distance utilities.
- Ray-casting sign estimator for watertight meshes.
- `ADASDF_DENSE_SDFBIN_V1` read/write support.
- `adasdf_build_dense_sdf` CLI.
- Public STL-to-SDF workflow documentation.
- Adaptive builder interface preview.
- `adasdf_build_adaptive_sdf_preview` dry-run CLI.
- DenseSDF roundtrip, query, collision, CLI, and adaptive-preview tests.

### Changed

- Version updated to `1.5.0-alpha`.
- `adasdf_info`, `adasdf_query`, `adasdf_collide`, expansion quality, and
  benchmark model loading can use DenseSDF `.sdfbin` files.
- Build-tree CLI outputs are normalized for core command-line tools.

### Notes

- DenseSDF builder is implemented and core-free.
- Adaptive octree/block/low-rank builder is interface preview only in
  `v1.5.0-alpha`.
- Full adaptive construction remains planned for v1.6/v1.7.
- Surrogate-guided parameter recommendation remains planned for v1.8.

## 1.4.0-alpha

Safe mesh cleanup and clean STL export.

### Added

- Safe `MeshCleanup` module.
- ASCII `STLWriter`.
- `adasdf_mesh_clean` CLI.
- `adasdf_mesh_check --clean-out` and `--clean-report`.
- Before/after cleanup diagnostics and readiness reports.
- Mesh cleanup demo example.
- Cleanup roundtrip tests.

### Changed

- Version updated to `1.4.0-alpha`.
- README, roadmap, capability matrix, tooling docs, install validation, and
  alpha validation now include the safe cleanup workflow.

### Notes

- This release does not implement hole filling or self-intersection repair.
- Cleanup is a preflight utility for future SDF construction, not an industrial
  CAD repair engine.
- Input STL files are never modified in place; cleaned STL files are explicit
  outputs.

## 1.3.0-alpha

Mesh build readiness and repair suggestions.

### Added

- `MeshReadiness` module for SDF build suitability assessment.
- Severity-ranked mesh issues with info, warning, and critical levels.
- 0-100 readiness score and `Ready`, `UsableWithWarnings`, `Poor`, and
  `NotRecommended` levels.
- Recommended preprocessing and repair suggestions for common STL issues.
- `adasdf_mesh_check --readiness`, `--strict`, `--lenient`,
  `--require-watertight`, and `--allow-open`.
- Combined diagnostics + readiness Markdown and JSON-like report output.
- Mesh readiness docs, release draft, tests, and validation coverage.

### Changed

- Version updated to `1.3.0-alpha`.
- Install validation and alpha validation now require readiness report output.
- README, roadmap, capability matrix, and public positioning docs now include
  SDF build readiness.

### Notes

- This is a readiness and suggestion release, not an automatic mesh repair
  engine.
- Input STL files are never modified.
- Mesh repair, self-intersection detection, and standalone arbitrary-STL SDF
  building remain planned work.

## 1.2.0-alpha

STL mesh diagnostics and import audit.

### Added

- ASCII / binary STL reader for diagnostics.
- Triangle mesh diagnostics module.
- Detection of degenerate triangles, duplicate triangles, boundary edges,
  non-manifold edges, connected components, isolated vertices, and scale
  warnings.
- Markdown and JSON-like diagnostics reports.
- `adasdf_mesh_check` CLI.
- Mesh diagnostics demo example.
- Project-generated diagnostic STL fixtures.

### Changed

- Version updated to `1.2.0-alpha`.
- Capability, positioning, roadmap, README, install validation, and alpha
  validation docs now include the STL diagnostics preflight path.

### Notes

- This is a preflight diagnostics release.
- It does not yet implement a full standalone arbitrary-STL adaptive SDF
  builder.
- Mesh repair and self-intersection detection remain planned work.

## 1.1.1-alpha

Capability exposure and public positioning release.

### Added

- Capability matrix.
- Implemented / partial / planned feature overview.
- FCL complement strategy documentation.
- Query backend matrix.
- Contact output matrix.
- Public positioning guide.
- `adasdf_capabilities` CLI.
- Capability walkthrough example.

### Changed

- Version updated to `1.1.1-alpha`.
- Reworked README to better expose implemented features and planned roadmap.

### Notes

- No core algorithm changes.
- No public API breaking changes.
- AdaSDF-CL remains an FCL-style SDF backend under development, not a drop-in FCL replacement.

## 1.1.0-alpha

Expanded-SDF accuracy audit, sign-mismatch metrics, and sampled existing-core
expansion bridge.

### Added

- `ExpansionQuality` module for deterministic direct-vs-expanded SDF quality
  audits.
- Sign classification helpers and strict sign-mismatch metrics with ambiguous
  sign accounting.
- Near-surface sign-mismatch reporting for contact-risk interpretation.
- Expanded `ExpansionOptions` for global/block resolution, padding,
  near-surface band, sign epsilon, quality-audit settings, outside-domain
  clamping, and direct fallback policy.
- `adasdf_expansion_quality` CLI for `.sdfbin` quality audits with CSV output.
- Benchmark CSV quality fields: max/mean/RMS/p95 error, sign mismatch,
  ambiguous sign, near-surface sign mismatch, fallback count, and fallback rate.
- Query-mode demo `--quality-audit` options.
- Tests for expansion quality, sign metrics, resolution control,
  existing-core sampled expansion bridge behavior, and the quality CLI.
- Expanded-SDF accuracy, sign-mismatch, existing-core bridge, v1.1 report, and
  release-draft documentation.

### Changed

- Version updated to `1.1.0-alpha`.
- Block-expanded queries choose overlapping blocks by smaller cell size, then
  deterministic block id priority.
- ExpandedSDF can carry query policy for clamp/fallback interpretation.
- Existing-core models that expose `SDFModel::sampleDistance()` can be expanded
  through the same sampled `SDFExpander` bridge as demo models.

### Notes

- CUDA remains optional and is not required for v1.1 quality audits.
- Existing-core tests gracefully skip when the existing core or fixture is not
  available.
- v1.1 expands existing-core models by sampling into ExpandedSDF layouts; it
  does not implement native low-rank compressed GPU query.

## 1.0.3-alpha

CUDA full-query benchmark and expanded-SDF query-path optimization.

### Added

- `QueryOutputMode` with explicit `PhiOnly` and `PhiAndNormal` query output modes.
- Benchmark `--output phi` and `--output phi,normal` options; legacy `--phi-only` remains an alias for `--output phi`.
- Benchmark `--device-only` / `--no-download` mode for CUDA-side timing without D2H result download or correctness checks.
- CSV fields for `output_mode`, workspace reuse, allocation count, workspace device memory, block lookup counters, download status, correctness status, host memory, and layout.
- Tests for benchmark output modes, device-only benchmark behavior, workspace reuse fields, CUDA output modes, and CUDA block lookup fast path.
- v1.0.3 performance optimization report and release draft.

### Changed

- Version updated to `1.0.3-alpha`.
- CPU direct batch query can skip gradient/normal work for `PhiOnly` output.
- CUDA resident expanded SDF queries can write into reusable output storage and can skip D2H result downloads for device-only benchmark runs.
- CUDA block finite-difference neighbor lookup avoids rescanning the already-tested center block during fallback.
- Benchmark all-skipped CUDA rows now exit successfully unless a row actually fails, preserving CPU-only CI behavior.

### Notes

- CUDA remains optional and disabled by default.
- This release does not implement full low-rank compressed SDF native GPU query.
- `--device-only` is a performance-analysis mode, not a correctness mode.
- Selected block benchmarks are meaningful for local contact-region point distributions; they are not a global uniform-query accelerator.

## 1.0.2-alpha.1

CI portability hotfix for v1.0.2-alpha.

### Fixed

- Fixed Linux GitHub Actions benchmark CLI tests by invoking benchmark executables with a portable explicit path.
- Preserved Windows benchmark test behavior.
- Kept CUDA benchmark semantics, resident query workspace, warmup/repeat, kernel-only, phi-only, and reuse-resident functionality unchanged.

### Notes

- No core CUDA algorithm changes.
- No public API changes.
- The original `v1.0.2-alpha` tag is retained for traceability but should not be used as the recommended public pre-release.

## 1.0.2-alpha

CUDA benchmark semantics and resident query workspace.

### Added

- Formal `BatchQueryTiming` breakdown for setup, expansion, SDF upload, query allocation, H2D points, CUDA kernel, synchronization, D2H results, CPU postprocess, free, and total time.
- Benchmark `--warmup`, `--repeat`, `--kernel-only`, `--reuse-resident`, and `--phi-only` modes.
- CSV timing breakdown and min/mean/max/std fields while retaining `query_kernel_ms` and `query_total_ms` compatibility columns.
- `CudaQueryWorkspace` for reusable CUDA query points, phi, and normal buffers.
- CUDA expanded-SDF phi-only kernel for signed-distance-only kernel timing.
- Benchmark and CUDA workspace/phi-only tests with graceful CUDA-unavailable skip behavior.
- CUDA benchmark semantics, GPU backend, diagnosis, release-draft, and v1.0.2 report documentation.

### Changed

- Version updated to `1.0.2-alpha`.
- CUDA expanded-SDF block lookup now directly handles single-block global dense layouts and prefers the center block for finite-difference neighbor samples before falling back to block scanning.
- Benchmark documentation now states that `total_ms` is not comparable to original UI warmed kernel averages; `--kernel-only --phi-only --reuse-resident` is the closest comparison.

### Notes

- CUDA remains optional and disabled by default.
- This release does not implement full low-rank compressed SDF native GPU query.
- Resident query workspace reduces repeated query-buffer allocation, but the expanded SDF values are still pre-expanded on CPU before upload.

## 1.0.1-alpha

Query mode, expansion mode, and GPU-resident expanded SDF query backend.

### Added

- `QueryModeConfig` for explicit backend, expansion, block selection, CPU fallback, and resident-data settings.
- `ExpandedSDF`, `SDFExpander`, and `QueryEngine` for CPU direct, CPU global-expanded, CPU block-expanded, CUDA global-expanded, and CUDA block-expanded query paths.
- CUDA resident expanded SDF upload/query/release path for pre-expanded global and block dense SDF data.
- `adasdf_query_mode_demo` CLI for CPU/CUDA query-mode smoke testing.
- `adasdf_collide --backend`, `--expansion`, and `--blocks` options wired into collision requests.
- Benchmark CLI flags for query backend, expansion mode, selected blocks, global/block resolution, resident data, and richer CSV fields.
- Tests for query-mode validation, block selection, SDF expansion, CPU QueryEngine, CUDA QueryEngine, and the query-mode CLI.
- Query-mode, GPU backend, benchmark, original UI CUDA audit, validation, release-draft, and v1.0.1 summary documentation.

### Changed

- Version updated to `1.0.1-alpha`.
- `CollisionRequest` and `DistanceRequest` now carry optional query-mode configuration while preserving CPU direct defaults.
- CUDA query validation now rejects compressed-direct mode with a clear error and requires `Global` or `Block` expansion.
- Benchmark output now reports memory, setup time, CUDA kernel time, total query time, fallback count, numerical error, CUDA availability, status, and error message.

### Notes

- CUDA remains optional and disabled by default.
- CUDA compressed-direct query is not implemented; `CUDA + None` is intentionally invalid.
- CUDA pair collision still uses the existing CPU narrow-phase path after request validation. This release does not claim an industrial GPU contact solver.
- The resident CUDA backend currently targets pre-expanded analytic/demo adaptive box SDF data. Full low-rank compressed SDF GPU expansion remains future work.

## 1.0.0-alpha

Optional CUDA batch query backend and CPU/GPU benchmark tooling.

### Added

- CPU batch signed-distance, gradient, and normal query API.
- Optional CUDA batch query backend for the core-free analytic/demo adaptive box path.
- CUDA box signed-distance and normal kernels compiled only when CUDA is explicitly enabled and available.
- Deterministic benchmark point-cloud generator with volume, near-surface, and mixed distributions.
- `adasdf_benchmark_batch_query` CLI for 10k/100k/1M CPU and optional CUDA benchmarking.
- CPU/GPU alignment and graceful CUDA-unavailable tests.
- Benchmarking, GPU backend, release draft, and benchmark report templates.

### Changed

- Version updated to `1.0.0-alpha`.
- CMake now reports CUDA backend availability, CUDA toolkit discovery, and benchmark build status.
- Alpha validation now runs the batch query benchmark and marks CUDA GPU rows as skipped when unavailable.

### Notes

- CUDA remains optional. CPU-only configure/build/test/install remains supported.
- The v1.0.0-alpha CUDA path initially supports analytic/demo adaptive boxes.
- Full low-rank compressed SDF GPU expansion and an industrial GPU collision pipeline remain future work.

## 0.9.0-alpha

Surrogate-guided adaptive demo workflow and contact visualization.

### Added

- Experimental demo surrogate recommender.
- Demo adaptive SDF builder using analytic box queries and adaptive metadata.
- Demo adaptive `.sdfbin` format `ADASDF_DEMO_ADAPTIVE_SDFBIN_V1`.
- `adasdf_recommend_demo` CLI.
- `adasdf_build_demo_adaptive` CLI.
- `adasdf_collide_boxes_demo` CLI with SVG output.
- Pure SVG collision visualization writer.
- Examples for surrogate-guided adaptive build and two-box collision with view.
- Tests for demo recommender, adaptive metadata, adaptive sdfbin, max contacts, and SVG output.

### Changed

- Version updated to `0.9.0-alpha`.
- README now documents the v0.9 clone-only surrogate-guided demo workflow.
- Alpha validation now runs recommendation, adaptive build, info, query, collision, SVG checks, and clean checks.
- `adasdf_collide` now reports requested and returned contact counts.

### Notes

- The v0.9 surrogate is experimental demo infrastructure only.
- It is not universal, not fully trained, and not an optimality guarantee.
- Full adaptive STL-to-compressed-SDF construction remains future work for the public standalone backend.

## 0.8.0-alpha

Core-free standalone demo backend.

### Added

- Analytic box SDF model.
- Core-free demo `.sdfbin` format.
- `adasdf_make_demo_box` CLI.
- Core-free point query and collision demo.
- Demo `.sdfbin` reader/writer.
- Core-free demo collision example.
- Tests for analytic SDF, demo `.sdfbin`, make-demo-box CLI, and core-free collision.

### Changed

- README now includes a clone-only demo workflow.
- CLI documentation includes the core-free demo workflow.
- Alpha validation now runs the v0.8 make/info/query/collide demo path without the existing core.

### Notes

- This is a demo backend for public integration and learning.
- It is not a replacement for the full adaptive compressed SDF builder.
- Full adaptive STL-to-sdfbin construction remains future work for the public standalone backend.

## 0.7.0-alpha.2

Documentation hotfix after external downstream collision testing.

### Changed

- Clarified the difference between core-free public builds and existing-core enhanced builds.
- Documented that core-free builds can compile, install, and link, but cannot yet generate/read real `.sdfbin` models.
- Added external collision test summary.
- Updated README limitations for public users.

### Notes

- No core algorithm changes.
- No public API changes.
- No CMake install/export changes.

## 0.7.0-alpha.1

CI-fixed alpha / research-preview release candidate.

### Fixed

- Fixed Ubuntu install validation portability.
- Improved downstream package validation for Linux single-config generators and Windows multi-config generators.
- Preserved Windows Visual Studio compatibility.

### Notes

- No core algorithm changes.
- No public API changes.
- `v0.7.0-alpha` is retained as the initial preflight tag but should not be used as the recommended public release.

## 0.7.0-alpha

External integration and packaging alpha.

### Added

- Install-tree validation script and summary report generation.
- CMake package-config smoke test under `tests/package`.
- Standalone downstream CMake example that links `AdaSDFCL::adasdf_cl`.
- Installed command-line tools: `adasdf_info`, `adasdf_query`, and `adasdf_collide`.
- CPack source archive configuration.
- External integration documentation.
- CI install/downstream smoke validation.
- Alpha validation `--include-install` mode.

### Changed

- Installed `AdaSDFCL::adasdf_cl` now resolves to the runtime static library.
- Header-only interface is exported as `AdaSDFCL::adasdf_cl_headers`.
- Repository clean checks catch install trees, package archives, CMake generated
  files, and downstream/package build directories.

### Known Limitations

- Existing `.sdfbin` point sampling still requires a build with the optional
  existing-core bridge.
- CUDA, FCL ABI compatibility, Python bindings, and real surrogate inference
  remain out of scope for this alpha.

## 0.6.0-alpha

Initial alpha / research-preview release candidate.

### Added

- Adaptive SDF builder bridge.
- `.sdfbin` reader/writer round-trip.
- `SDFModel` point distance and gradient query.
- FCL-style `CollisionObject`, `collide`, and `distance` APIs.
- CPU SDF-sampling narrow-phase backend.
- Symmetric contact generation.
- Deterministic contact reduction.
- `adasdf_build` CLI.
- Validation card documentation.
- Repository clean check script.
- Alpha validation script.
- GitHub CI and issue/PR templates.
- CMake install/export package skeleton.

### Known Limitations

- Pair collision is an approximate SDF-sampling narrow-phase.
- CUDA batched pair query is not implemented.
- FCL ABI compatibility is not provided.
- Surrogate recommender is an API placeholder unless a backend is explicitly connected.
- Contact-only `.sdfbin` remains planned.
