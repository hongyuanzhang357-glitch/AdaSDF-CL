# Changelog

## 1.16.0-alpha.3

Contact-focused narrow-band SDF construction for collision-oriented use cases.

### Added

- Contact-band SDF sampling mode.
- Distance-aware and hybrid contact-band marker.
- Contact-band exact sampling with far-field interpolation.
- Contact-band quality audit for phi, sign and normal.
- Coverage audit to detect missed contact-band points.
- Marker cost audit and timing semantics.
- End-to-end timing fields and performance claim gate.
- Synthetic fixtures for thin gaps, robot-like links, dense curved meshes and
  gear-like teeth.
- `adasdf_benchmark_contact_band_sampling`.
- `adasdf_sweep_contact_band_sampling`.
- Python wrapper support for contact-band benchmark.

### Improved

- Reduces exact node sampling in collision-relevant construction paths.
- Preserves contact-band phi/sign/normal quality on tested synthetic fixtures.
- Distinguishes end-to-end speedup from diagnostic-only speedup.

### Notes

- The default builder path remains exact unless contact-band sampling is
  explicitly requested.
- Far-field phi is relaxed and should not be interpreted as globally
  high-accuracy SDF reconstruction.
- Speedup claims apply only to contact-focused, collision-oriented construction
  cases where quality and coverage gates pass.
- Old tags are preserved.

## 1.16.0-alpha.2

Hierarchical sampling diagnostics and overhead reduction.

### Added

- Detailed hierarchical sampling diagnostics for block classes, accepted
  predictions, fallbacks, exact BVH samples, quality-check samples, coarse
  sample reuse, sampler counters, timing buckets, and speedup claim gating.
- `--far-field-quality-check none|corners|sparse|full`,
  `--far-field-safety-factor`,
  `--transition-quality-check-samples`,
  `--hierarchical-diagnostics`, and `--no-hierarchical-diagnostics` on
  `adasdf_benchmark_hierarchical_sampling`.
- `--diagnostics-report` and `--diagnostics-csv` benchmark outputs.
- `adasdf_sweep_hierarchical_sampling` for quality-preserving parameter
  sweeps.
- Project-generated `wavy_sphere_ascii.stl` synthetic fixture.

### Improved

- Near-surface exact blocks now skip the unnecessary coarse probe path.
- Non-near blocks reuse one coarse sample grid for classification and
  prediction.
- Far-field blocks default to corner quality checks instead of dense checks.
- Effective acceleration claims are gated on both `speedup > 1` and
  `quality_gate_passed=true`.

### Notes

- Quality gates remain mandatory.
- Near-surface blocks remain exact by default.
- The measured alpha.2 smoke benchmarks improve over alpha.1 but still do not
  demonstrate effective construction-speed acceleration.

## 1.16.0-alpha.1

Tag/documentation hotfix for the v1.16 hierarchical adaptive sampling release.

### Fixed

- Added a hotfix tag that includes the final v1.16 documentation/report commit.
- Preserved the original v1.16.0-alpha tag without moving it.

### Notes

- No algorithmic change intended.
- Use v1.16.0-alpha.1 for the GitHub pre-release.

## 1.16.0-alpha

Hierarchical adaptive sampling with quality guard.

### Added

- `include/adasdf/sampling` and `src/sampling` for block classification,
  sampling policy, coarse prediction, sampling quality guard, hierarchical
  block sampling, speed/quality benchmarking, and report writing.
- `--sampling exact|hierarchical` and related guarded sampling options on
  adaptive and compressed builder CLIs.
- `adasdf_benchmark_hierarchical_sampling`.
- Python wrapper support and tests for hierarchical sampling commands.
- Validation coverage for the hierarchical sampling benchmark.

### Notes

- Exact sampling remains the default.
- Predictions require quality guard and exact fallback.
- `.sdfbin` formats are unchanged.

## 1.15.0-alpha

Strict JSON/CSV reporting and reproducibility manifests.

### Added

- `include/adasdf/report` and `src/report` with strict schema, run/case
  manifest, strict JSON writer, strict CSV writer, report validator, and run
  summary collector.
- CLIs: `adasdf_write_manifest`, `adasdf_validate_report`, and
  `adasdf_collect_run_summary`.
- `--strict-json` and `--case-id` support on selected build, query, benchmark,
  and world CLIs.
- Python wrapper helpers for manifest write, report validation, run summary
  collection, and strict report argument forwarding.
- Strict report docs, release draft, alpha validation coverage, and install
  validation coverage.

### Notes

- Report code is dependency-free and does not require nlohmann_json, Python,
  FCL, CUDA, or the existing research core.
- Core collision algorithms are unchanged.

## 1.14.0-alpha

CollisionWorld broadphase and multi-object sample-based SDF collision.

### Added

- `WorldTransform`, `CollisionGroupMask`, `WorldObject`, and
  `CollisionWorld`.
- Deterministic `AABBBroadphase` for enabled-state, static-static,
  group/mask, and AABB overlap filtering.
- `WorldSceneIO` for simple CSV world scenes.
- `WorldSparseCollision` for broadphase pairs plus existing sparse SDF
  narrowphase.
- `WorldSolverContacts` and `WorldReportWriter` for solver-ready world contact
  candidate export.
- CLIs: `adasdf_world_broadphase`, `adasdf_world_sparse_collide`,
  `adasdf_world_solver_contacts`, and `adasdf_benchmark_collision_world`.
- Python wrapper helpers for the new world CLIs.
- CollisionWorld scene format docs, v1.14 report, release draft, and
  validation coverage.

### Changed

- Version updated to `1.14.0-alpha`.
- Install and alpha validation now exercise the world CLIs with generated
  validation scenes outside the source tree.
- Capability, status, roadmap, positioning, and README docs now mark
  CollisionWorld broadphase as implemented.

### Notes

- World narrowphase is sample-based SDF collision, not exact mesh-vs-mesh
  contact.
- Solver contacts are candidates, not solver constraints or solver impulses.
- v1.14 does not implement FCL fallback, CCD, ROS/MoveIt, pybind11/native
  Python bindings, or a general scene graph.

## 1.13.0-alpha.2

CI hotfix release for the v1.13 tag workflow.

### Fixed

- Limited GitHub Actions build parallelism to reduce Ubuntu runner peak load
  when `main` and tag workflows are triggered for the same commit.
- Preserved the `v1.13.0-alpha` and `v1.13.0-alpha.1` tags without moving
  either tag.

### Notes

- No algorithmic changes intended.
- `v1.13.0-alpha.1` passed on `main` but its tag workflow was cancelled during
  the Ubuntu build step.
- Use `v1.13.0-alpha.2` for the green CI pre-release.

## 1.13.0-alpha.1

CI hotfix release.

### Fixed

- Fixed GitHub Actions compatibility for the v1.13 contact stabilization
  release by reusing the already-built CI tree during install validation
  instead of rebuilding the full project in the Ubuntu job.
- Aligned CI configure options with install validation options so the reused
  build directory remains compatible across validation steps.
- Preserved the `v1.13.0-alpha` tag without moving it.

### Notes

- No algorithmic changes intended.
- `v1.13.0-alpha` remains unchanged.
- Superseded by `v1.13.0-alpha.2` after the `v1.13.0-alpha.1` tag workflow
  was cancelled during the Ubuntu build step.

## 1.13.0-alpha

Solver-aware contact candidate stabilization.

### Added

- `ContactBudget` for hard-contact candidate limits.
- `ContactPatch` and `ContactClusterer`.
- `ContactStabilizer` with duplicate removal, thresholding, clustering, and
  deterministic budget enforcement.
- `SolverContact` and `SolverContactSet` export.
- Solver contact CSV, Markdown, and JSON-like writers.
- `adasdf_stabilize_contacts` CLI.
- `adasdf_solver_contact_candidates` CLI.
- `adasdf_benchmark_contact_reduction` CLI.
- Python wrapper support for stabilized solver contacts.
- Determinism and contact budget tests.

### Notes

- Solver-ready contacts are not solver impulses.
- v1.13 does not implement a contact solver.
- Contact budget is important for hard-contact dynamics.
- Temporal coherence and warm-start tracking remain planned work.

## 1.12.0-alpha

BVH-accelerated SDF builder sampling and deterministic parallel sampling.

### Added

- CPU `TriangleAABB`, `TriangleBVH`, and deterministic median-split
  `TriangleBVHBuilder`.
- `BVHNearestTriangleQuery`, `BVHRayIntersectionQuery`, and `BVHSDFSampler`.
- `ParallelSampling` stdlib helper for deterministic multi-threaded sampling.
- Builder options and CLI flags: `--accel brute|bvh`, `--threads N`, and
  `--benchmark-brute-reference`.
- `adasdf_benchmark_builder_acceleration` CLI.
- Python wrapper parameters and parsers for builder acceleration.
- BVH acceleration docs, benchmark docs, and v1.12 release draft.

### Changed

- Version updated to `1.12.0-alpha`.
- DenseSDF, AdaptiveBlockSDF, and CompressedAdaptiveBlockSDF builders can use
  optional BVH sampling while keeping brute force as the default reference.

### Notes

- v1.12 is CPU-only builder acceleration. It is not GPU BVH, not FCL fallback,
  not CollisionWorld, and not a low-rank math change.
- CPU-only builds remain fully supported.

## 1.11.0-alpha

CUDA active block cache and GPU local active-block query.

### Added

- `CudaActiveBlockBuffer` for uploading CPU-expanded active dense blocks.
- `CudaActiveBlockCache` for coordinating CPU active expansion and CUDA upload.
- `CudaActiveBlockQuery` for phi-only and phi+normal sparse active-block
  queries on GPU.
- `CudaActiveBlockBenchmark` for comparing CUDA active block query timing
  against CPU active block and direct sparse query baselines.
- `adasdf_cuda_active_block_query` CLI.
- `adasdf_benchmark_cuda_block_cache` CLI.
- Python wrappers for CUDA active block query and CUDA block cache benchmark.
- CUDA active block API, CLI, Python contract, and CPU-only unavailable-path
  tests.

### Changed

- Version updated to `1.11.0-alpha`.
- Validation scripts now discover the CUDA active block tools and treat return
  code `20` as a successful CUDA-unavailable skip.

### Notes

- v1.11 selects active blocks on CPU, expands active blocks on CPU, uploads
  expanded local dense blocks to GPU, and queries those local dense blocks with
  CUDA.
- v1.11 is not GPU-native compressed SVD reconstruction and does not do
  per-point low-rank factor reconstruction on GPU.
- CUDA remains optional. CPU-only builds and install validation remain
  supported.
- `adasdf_cuda_active_block_query` returns code `10` when collision is
  detected and code `20` when CUDA is unavailable.

## 1.10.0-alpha

Contact-aware active block expansion and CPU expanded-block cache.

### Added

- `ActiveBlockSelector` for sparse contact-aware block id selection.
- `ActiveExpandedBlock` runtime block type.
- `ExpandedBlockCache` with LRU residency and memory statistics.
- `BlockExpansionManager` for one-time adaptive/compressed block expansion.
- `ActiveBlockQuery` with cache/fallback source reporting.
- `ActiveBlockReportWriter` for CSV, Markdown, and JSON-like output.
- `adasdf_select_active_blocks` CLI.
- `adasdf_active_block_query` CLI.
- `adasdf_benchmark_block_cache` CLI.
- Python wrappers for active block selection, active block query, and block
  cache benchmarking.
- Active block cache tests and validation script coverage.

### Changed

- Version updated to `1.10.0-alpha`.
- Validation scripts now exercise active block CLI tools.
- Capability, query backend, runtime memory, and Python wrapper docs now
  document active block cache as implemented.

### Notes

- v1.10 implements CPU active block cache.
- CUDA active block cache remains planned.
- GPU-native compressed SDF query remains planned.
- `adasdf_active_block_query` return code `10` means collision detected; it is
  not a program failure.
- The active cache expands selected local blocks and does not perform global
  expansion.

## 1.9.0-alpha

Sparse SDF collision query and contact candidate API.

### Added

- `CollisionSampleSet` and CSV I/O.
- Sparse point-to-SDF query API.
- Collision-only, clearance, and candidate-search sparse query modes.
- Sample radius support for point/sphere proxy collision through
  `effective_phi = phi - radius`.
- Early-exit sparse collision checks.
- Top-K contact candidate reduction with optional `reduction_radius`.
- Sparse query and candidate CSV, Markdown, and JSON-like report writers.
- `adasdf_sparse_query` CLI.
- `adasdf_sparse_collide` CLI.
- `adasdf_contact_candidates` CLI.
- `adasdf_benchmark_sparse_query` CLI.
- Python wrapper support for sparse query, sparse collision, contact
  candidates, and sparse query benchmarking.
- Sparse collision tests and project-generated sample fixtures.

### Changed

- Version updated to `1.9.0-alpha`.
- Validation scripts now exercise sparse query, sparse collision, contact
  candidates, sparse benchmark, and Python sparse wrappers.
- Capability, query backend, contact output, FCL complement, positioning, and
  Python wrapper docs now document sparse point collision as a first-class use
  case.

### Notes

- Sparse query defaults to phi-only and does not compute normals unless
  requested.
- `adasdf_sparse_collide` return code `10` means collision detected; it is not
  a program failure.
- Contact candidates are not full solver contacts and are not a complete
  contact manifold.
- Hard-contact workflows should keep contact budgets small.
- Direct compressed query is appropriate for sparse queries, debugging,
  fallback, and small point sets, but not the main high-throughput GPU path.
- Contact-aware active block expansion/cache is planned for v1.10.

## 1.8.1-alpha

Python CLI wrapper.

### Added

- Pure-Python `adasdf_cli` wrapper package.
- Subprocess runner for installed AdaSDF-CL CLI tools.
- Python helpers for mesh check, mesh clean, build recommendation, DenseSDF
  build, AdaptiveBlockSDF build, compressed SDF build, info, query, collide,
  expansion quality, and benchmark.
- Python dataclass result types that preserve command, stdout, stderr,
  returncode, and elapsed time.
- Best-effort output parsers for recommendation, query, collision, info, and
  benchmark output.
- Python `unittest` coverage and optional real-CLI smoke tests.
- Python wrapper documentation and v1.8.1 release draft.

### Changed

- Version updated to `1.8.1-alpha`.
- Validation scripts now run Python wrapper tests with installed CLI tools.
- Capability and positioning docs distinguish the implemented CLI wrapper from
  planned native Python / pybind11 bindings.

### Notes

- This is a subprocess-based convenience layer.
- It is not a native pybind11 binding.
- It is not a C++ extension module.
- It requires installed AdaSDF-CL CLI tools.
- It uses only the Python standard library at runtime.
- Native Python bindings remain planned work.

## 1.8.0-alpha

Surrogate-guided build recommendation for public core-free STL workflows.

### Added

- `adasdf_recommend_build` CLI.
- `adasdf::MeshFeatureExtractor`.
- `adasdf::BuildSurrogateEstimator`.
- `adasdf::SurrogateProfile`.
- `adasdf::BuildRecommender`.
- `adasdf::BuildRecommendationWriter`.
- Markdown and JSON-like recommendation reports.
- Optional key-value recommendation profiles.
- `examples/20_build_recommendation_demo.cpp`.
- Recommendation workflow, schema, profile, and release docs.

### Changed

- Version updated to `1.8.0-alpha`.
- Adaptive builder preview now marks deterministic surrogate recommendation as
  implemented and trained surrogate models as planned.
- `adasdf_build_dense_sdf`, `adasdf_build_adaptive_sdf`, and
  `adasdf_build_compressed_sdf` accept `--recommend` as a non-building hint.
- Capability matrix and validation scripts now include `adasdf_recommend_build`.

### Notes

- The recommender is deterministic and experimental.
- It is not a universal trained model.
- It is not fully trained.
- It is not an optimality guarantee.
- It does not build or write `.sdfbin` output by default.
- Full low-rank compressed SDF GPU-native query remains planned.

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
