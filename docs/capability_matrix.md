# Capability Matrix

Status values: Implemented, Partial, Experimental, Demo-only, Existing-core
only, Planned, Not implemented.

| Category | Capability | Status | Available since | User-facing API/Tool | Notes |
| --- | --- | --- | --- | --- | --- |
| Build / Input | core-free analytic demo box | Implemented | v0.8 | `AnalyticSDFModel`, `adasdf_make_demo_box` | Public standalone path. |
| Build / Input | demo adaptive SDF | Implemented | v0.9 | `DemoAdaptiveSDFBuilder`, `adasdf_build_demo_adaptive` | Demo analytic backend with adaptive metadata. |
| Build / Input | STL reader | Implemented | v1.2 | `STLReader` | Diagnostics-oriented reader. |
| Build / Input | ASCII STL | Implemented | v1.2 | `STLReader` | Common ASCII STL syntax. |
| Build / Input | Binary STL | Implemented | v1.2 | `STLReader` | Validates binary triangle count against file size. |
| Build / Input | ASCII STL writer | Implemented | v1.4 | `STLWriter` | Writes cleaned TriangleMesh output; binary writing remains planned. |
| Build / Input | existing-core STL-to-sdfbin bridge | Existing-core only | v0.4 | `AdaptiveSDFBuilder`, `adasdf_build` | Requires existing research core. |
| Build / Input | standalone uniform STL-to-DenseSDF builder | Implemented | v1.5 | `DenseSDFBuilder`, `adasdf_build_dense_sdf` | Core-free baseline builder; brute-force uniform grid. |
| Build / Input | adaptive octree builder | Implemented | v1.6 | `AdaptiveOctreeBuilder` | Deterministic near-surface refinement baseline. |
| Build / Input | adaptive block partitioner | Implemented | v1.6 | `AdaptiveBlockPartitioner` | One dense block per octree leaf. |
| Build / Input | standalone adaptive block STL builder | Implemented | v1.6 | `AdaptiveBlockSDFBuilder`, `adasdf_build_adaptive_sdf` | Core-free adaptive octree/block builder; block data remains dense. |
| Build / Input | adaptive compressed builder interface preview | Experimental | v1.5 | `AdaptiveSDFBuilderPreview`, `adasdf_build_adaptive_sdf_preview` | Planning tool; matrix-SVD and deterministic recommendation stages are implemented, trained surrogate and GPU compressed stages remain planned. |
| Build / Input | low-rank adaptive compressed STL builder | Implemented | v1.7 | `adasdf_build_compressed_sdf`, `adasdf_compress_adaptive_sdf` | Matrix-SVD block compression with dense fallback and quality audit. |
| Build / Input | hierarchical adaptive block sampling with quality guard | Experimental | v1.16 | `--sampling hierarchical`, `HierarchicalBlockSampler` | Default remains exact. Near-surface blocks stay exact by default; predicted blocks require quality guard and exact fallback. No `.sdfbin` format change. |
| Build / Input | contact-focused narrow-band sampling | Alpha available | v1.16.0-alpha.3 | `--sampling contact-band`, `ContactBandBlockSampler` | Collision-oriented construction path. Exact-samples the contact band, interpolates relaxed far-field values, audits contact-band phi/sign/normal quality and coverage. Not a global full-field reconstruction acceleration claim. |
| Build / Input | BVH-accelerated builder sampling | Implemented | v1.12 | `--accel bvh`, `TriangleBVH` | Optional CPU acceleration for DenseSDF, AdaptiveBlockSDF, and CompressedAdaptiveBlockSDF sampling; brute remains default. |
| Build / Input | deterministic parallel builder sampling | Implemented | v1.12 | `--threads N`, `ParallelSampling` | C++17 stdlib threads only; no OpenMP/TBB dependency. |
| Build / Input | surrogate-guided build recommendation | Experimental | v1.8 | `BuildRecommender`, `adasdf_recommend_build` | Deterministic heuristic-calibrated recommender for DenseSDF, AdaptiveBlockSDF, and CompressedAdaptiveBlockSDF. |
| Build / Input | mesh-feature-based parameter recommendation | Experimental | v1.8 | `MeshFeatureExtractor`, `BuildSurrogateEstimator` | Uses diagnostics/readiness features, target error, memory budget, and use case. |
| Build / Input | recommendation profile override | Experimental | v1.8 | `SurrogateProfile`, `--profile` | Key-value constants only; not a universal trained model. |
| Build / Input | trained surrogate model integration | Planned | - | - | Not implemented. |
| Build / Input | online recommendation calibration | Planned | - | - | Not implemented. |
| Build / Input | Tucker/HOSVD adaptive compression | Planned | - | - | Not implemented in v1.8. |
| Build / Input | mesh diagnostics | Implemented | v1.2 | `MeshDiagnostics`, `adasdf_mesh_check` | Preflight only, no repair. |
| Build / Input | mesh readiness scoring | Implemented | v1.3 | `MeshReadiness`, `adasdf_mesh_check --readiness` | Preflight suitability score and suggestions. |
| Build / Input | repair suggestions | Implemented | v1.3 | `MeshReadinessReport` | Text guidance only; input STL is not modified. |
| Build / Input | safe mesh cleanup | Implemented | v1.4 | `MeshCleanup`, `adasdf_mesh_clean`, `adasdf_mesh_check --clean-out` | Removes obvious duplicate/degenerate elements and writes separate cleaned STL. |
| Build / Input | complex mesh repair / hole filling | Planned | - | - | Not implemented. |
| Build / Input | unit / scale report | Partial | v0.9 | `adasdf_info` | Metadata shown when available. |
| SDF storage / model | demo text `.sdfbin` | Implemented | v0.8 | `DemoSDFBin` | Core-free analytic demo format. |
| SDF storage / model | demo adaptive `.sdfbin` | Implemented | v0.9 | `DemoAdaptiveSDFBin` | Core-free adaptive demo format. |
| SDF storage / model | dense `.sdfbin` | Implemented | v1.5 | `DenseSDFBin` | Text format `ADASDF_DENSE_SDFBIN_V1`. |
| SDF storage / model | uniform dense SDF model | Implemented | v1.5 | `DenseSDFModel` | Trilinear sampling and finite-difference gradients. |
| SDF storage / model | adaptive block `.sdfbin` | Implemented | v1.6 | `AdaptiveBlockSDFBin` | Text format `ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1`; dense phi per block. |
| SDF storage / model | adaptive block SDF model | Implemented | v1.6 | `AdaptiveBlockSDFModel` | Direct query, gradient, collision, expansion, and benchmark support. |
| SDF storage / model | compressed adaptive block `.sdfbin` | Implemented | v1.7 | `CompressedBlockSDFBin` | Text format `ADASDF_COMPRESSED_BLOCK_SDFBIN_V1`; mixed MatrixSVD and DenseFallback blocks. |
| SDF storage / model | compressed adaptive block SDF model | Implemented | v1.7 | `CompressedAdaptiveBlockSDFModel` | Direct CPU query reconstructs needed grid values on demand. |
| SDF storage / model | existing-core `.sdfbin` read | Existing-core only | v0.2 | `SDFBinReader` | Query requires existing core. |
| SDF storage / model | contact-only `.sdfbin` | Implemented | v0.6 | `ContactOnlySDFBin` | Lightweight contact export format. |
| SDF storage / model | existing-core low-rank compressed SDF native public reader | Partial | v0.2 | `CompressedSDF`, `SDFModel` | Existing-core bridge remains separate from the v1.7 core-free compressed block format. |
| Query | point signed distance | Implemented | v0.2 | `adasdf_query`, `SDFModel::sampleDistance` | Direct model query. |
| Query | gradient / normal | Implemented | v0.2 | `SDFModel::sampleGradient`, `sampleNormal` | Analytic or finite-difference backed. |
| Query | sparse point-to-SDF query | Implemented | v1.9 | `SparseSDFQuery`, `adasdf_sparse_query` | Defaults to phi-only; normals are computed only when requested. |
| Query | sample-radius sparse query | Implemented | v1.9 | `CollisionSample::radius` | Uses `effective_phi = phi - radius`. |
| Query | CPU direct | Implemented | v1.0.1 | `QueryModeConfig::cpuDirect` | Default stable route. |
| Query | compressed adaptive CPU direct | Implemented | v1.7 | `CompressedAdaptiveBlockSDFModel` | Matrix-SVD values are reconstructed on demand; dense fallback blocks query directly. |
| Query | CPU global expanded | Implemented | v1.0.1 | `SDFExpander`, `QueryEngine` | Dense global layout. |
| Query | CPU block expanded | Implemented | v1.0.1 | `BlockSelection`, `QueryEngine` | All or selected blocks. |
| Query | CUDA global expanded | Experimental | v1.0.1 | `CudaResidentExpandedSDF` | Optional CUDA. |
| Query | CUDA block expanded | Experimental | v1.0.1 | `CudaResidentExpandedSDF` | Optional CUDA, best for local points. |
| Query | CUDA compressed-direct | Planned | - | - | Not implemented. |
| Query | contact-aware active block expansion/cache | Implemented | v1.10 | `ActiveBlockSelector`, `ExpandedBlockCache`, `ActiveBlockQuery` | CPU local expansion cache for compressed/contact workflows. |
| Query | fast block lookup index | Alpha available | v1.17 | `BlockLookupIndex`, `--lookup linear\|hash\|morton` | Deterministic point-to-block lookup using linear reference, spatial hash, or Morton-sorted lookup. The block-internal eight trilinear interpolation nodes still use direct regular-grid indexing. |
| Query | active cache slot lookup | Alpha available | v1.17 | `ActiveBlockHashMap`, `ActiveBlockSpatialHash`, `CacheSlotMap`, `--cache-lookup linear\|hash\|spatial-hash` | Maps block_id to cache slot and point to active cache slot without scanning every resident active block. Linear fallback remains available. |
| Query | CUDA active block cache baseline | Experimental | v1.11 | `CudaActiveBlockCache`, `adasdf_cuda_active_block_query` | CPU selects and expands active blocks, then uploads expanded local dense blocks to CUDA for local block interpolation. Not GPU-native compressed SVD reconstruction. |
| Query | CUDA active block phi+normal query | Experimental | v1.11 | `CudaActiveBlockQuery` | Phi-only and phi+normal sparse query over uploaded active dense blocks; CUDA unavailable returns skip code `20` in CLI. |
| Query | block selection | Implemented | v1.0.1 | `BlockSelection` | Deterministic selected ids. |
| Query | fallback count | Implemented | v1.0.1 | `QueryEngineStats`, benchmark CSV | Tracks direct fallback where applicable. |
| Query | kernel-only benchmark | Implemented | v1.0.2 | `--kernel-only` | CUDA timing semantics. |
| Query | phi-only benchmark | Implemented | v1.0.2 | `--output phi`, `--phi-only` | UI kernel-comparison path. |
| Collision / Contact | FCL-style `CollisionObject` | Implemented | v0.3 | `CollisionObject` | API shape, not FCL ABI. |
| Collision / Contact | `collide()` | Implemented | v0.3 | `adasdf::collide` | Pair collision. |
| Collision / Contact | `distance()` | Implemented | v0.3 | `adasdf::distance` | Pair distance. |
| Collision / Contact | contact point | Implemented | v0.5 | `Contact::point` | Research-preview contact generation. |
| Collision / Contact | normal | Implemented | v0.5 | `Contact::normal` | Orientation is documented boundary. |
| Collision / Contact | penetration depth | Implemented | v0.5 | `Contact::penetration_depth` | SDF-derived. |
| Collision / Contact | signed distance | Implemented | v0.5 | `Contact::signed_distance` | Per contact. |
| Collision / Contact | max contacts | Implemented | v0.7 | `CollisionRequest::max_contacts` | Enforced in CLI/API. |
| Collision / Contact | deterministic contact reduction | Implemented | v0.5 | `ContactReducer` | Not full manifold clustering. |
| Collision / Contact | sparse collision-only early exit | Implemented | v1.9 | `SparseCollisionQuery`, `adasdf_sparse_collide` | Return code `10` means collision detected, not failure. |
| Collision / Contact | sparse clearance query | Implemented | v1.9 | `SparseCollisionMode::Clearance` | Full traversal, returns minimum effective phi. |
| Collision / Contact | sparse candidate search | Implemented | v1.9 | `SparseCollisionMode::CandidateSearch` | Returns threshold violations for reduction. |
| Collision / Contact | Top-K contact candidates | Implemented | v1.9 | `ContactCandidateReducer`, `adasdf_contact_candidates` | Deterministic candidate reduction with optional `reduction_radius`; not a full contact manifold. |
| Collision / Contact | solver-aware contact candidates | Implemented | v1.13 | `ContactStabilizer`, `adasdf_solver_contact_candidates` | Clusters and budgets SDF candidates into solver-ready contacts; not a contact solver and no impulses/friction. |
| Collision / Contact | contact patch clustering | Implemented | v1.13 | `ContactClusterer`, `ContactPatch` | Spatial and normal-consistent patches with deterministic representatives. |
| Collision / Contact | solver contact export | Implemented | v1.13 | `SolverContactExporter`, `adasdf_stabilize_contacts` | CSV, Markdown, and JSON-like solver-ready candidate export. |
| Collision / Contact | CollisionWorld container | Implemented | v1.14 | `CollisionWorld`, `WorldObject`, `WorldTransform` | Multi-object SDF world container with transforms, enabled state, object type, and samples. |
| Collision / Contact | AABB broadphase | Implemented | v1.14 | `AABBBroadphase`, `adasdf_world_broadphase` | Deterministic O(N^2) broadphase with disabled-object, static-static, group/mask, and AABB overlap filtering. |
| Collision / Contact | world sparse collision | Implemented | v1.14 | `WorldSparseCollision`, `adasdf_world_sparse_collide` | Sample-based SDF narrowphase over broadphase pairs; return code `10` means collision detected. |
| Collision / Contact | world solver-ready contacts | Implemented | v1.14 | `WorldSolverContacts`, `adasdf_world_solver_contacts` | Converts world sparse violations into solver-ready candidates; not constraints, impulses, or friction. |
| Collision / Contact | simple world scene CSV | Implemented | v1.14 | `WorldSceneIO` | Text scene format for SDF object paths, sample paths, transforms, groups, masks, and object type. |
| Collision / Contact | nearest points | Implemented | v0.3 | `DistanceResult` | Pair distance result. |
| Collision / Contact | contact manifold clustering | Partial | v0.5 | `ContactReducer` | Stable robot-grade manifold planned. |
| Collision / Contact | CCD | Planned | - | - | Not implemented. |
| Accuracy / Reliability | expansion quality audit | Implemented | v1.1.0 | `ExpansionQuality`, `adasdf_expansion_quality` | Direct vs expanded. |
| Accuracy / Reliability | max / mean / RMS / p95 error | Implemented | v1.1.0 | quality report, benchmark CSV | Deterministic samples. |
| Accuracy / Reliability | sign mismatch | Implemented | v1.1.0 | `sign_mismatch_rate` | Strict inside/outside mismatch. |
| Accuracy / Reliability | ambiguous sign | Implemented | v1.1.0 | `ambiguous_sign_rate` | Separate from mismatch. |
| Accuracy / Reliability | near-surface sign mismatch | Implemented | v1.1.0 | `near_surface_sign_mismatch_rate` | Contact-risk indicator. |
| Accuracy / Reliability | fallback statistics | Implemented | v1.1.0 | `fallback_count`, `fallback_rate` | Quality and benchmark. |
| Accuracy / Reliability | compression quality audit | Implemented | v1.7 | `CompressionQuality`, compression CLIs | Dense-adaptive vs compressed comparison with error and sign metrics. |
| Accuracy / Reliability | build recommendation confidence scoring | Experimental | v1.8 | `RecommendationConfidence` | Low/medium/high heuristic confidence; not a certification. |
| Accuracy / Reliability | memory/error build estimates | Experimental | v1.8 | `BuildSurrogateEstimator` | Deterministic estimates with safety factors; validate final SDF reports. |
| Accuracy / Reliability | strict JSON reproducibility report | Implemented | v1.15 | `StrictJsonWriter`, `--strict-json` | Dependency-free schema for selected build/query/benchmark/world CLIs. |
| Accuracy / Reliability | strict report validation | Implemented | v1.15 | `adasdf_validate_report` | Validates required fields and optional numeric metrics. |
| Accuracy / Reliability | run summary CSV collection | Implemented | v1.15 | `adasdf_collect_run_summary` | Collects strict reports into a fixed-column CSV. |
| Accuracy / Reliability | mesh AABB / scale report | Implemented | v1.2 | `MeshDiagnosticsReport` | Prompts users to confirm units. |
| Accuracy / Reliability | boundary / non-manifold edge report | Implemented | v1.2 | `MeshDiagnosticsReport` | Topology-level preflight. |
| Accuracy / Reliability | mesh issue severity classification | Implemented | v1.3 | `MeshIssueSeverity` | Info, warning, and critical. |
| Accuracy / Reliability | SDF build readiness score | Implemented | v1.3 | `MeshReadinessReport::score` | 0-100 heuristic, not certification. |
| Accuracy / Reliability | cleanup before/after report | Implemented | v1.4 | `MeshDiagnosticsWriter::cleanupComparisonMarkdown` | Compares diagnostics and readiness before and after safe cleanup. |
| Accuracy / Reliability | self-intersection detection | Planned | - | - | Not implemented. |
| Accuracy / Reliability | per-contact error bound | Planned | - | - | Not implemented. |
| Accuracy / Reliability | query confidence | Planned | - | - | Not implemented. |
| Integration | CMake install/export | Implemented | v0.7 | install targets | Public package. |
| Integration | `find_package(AdaSDFCL CONFIG REQUIRED)` | Implemented | v0.7 | downstream CMake | Tested. |
| Integration | downstream CMake example | Implemented | v0.7 | `examples/downstream_cmake_project` | No-input demo path. |
| Integration | Python CLI wrapper | Implemented | v1.8.1 | `python/adasdf_cli` | Pure-Python subprocess wrapper over installed CLI tools; not pybind11. |
| Integration | Python package distribution | Partial | v1.8.1 | `python/pyproject.toml` | Supports source-tree use and editable install; no pip release in v1.8.1. |
| Integration | Native Python / pybind11 binding | Planned | - | `PythonBindingPlan` | Not implemented. |
| Integration | C API | Planned | - | - | Not implemented. |
| Integration | ROS / MoveIt | Planned | - | - | Not implemented. |
| Integration | FCL fallback backend | Planned | - | - | Not implemented. |
| Integration | FCL adapter | Partial | v0.3 | `FCLAdapter` | API shape only. |
| Benchmark | CPU batch query | Implemented | v1.0 | `adasdf_benchmark_batch_query` | Deterministic. |
| Benchmark | CUDA batch query | Experimental | v1.0 | benchmark CLI | Optional CUDA. |
| Benchmark | warmup / repeat | Implemented | v1.0.2 | `--warmup`, `--repeat` | Repeat statistics. |
| Benchmark | kernel-only / total-time distinction | Implemented | v1.0.2 | CSV timing fields | Clear timing semantics. |
| Benchmark | sparse query benchmark | Implemented | v1.9 | `adasdf_benchmark_sparse_query` | CPU sparse phi-only, phi-normal, collision-only, clearance, and candidates modes. |
| Benchmark | active block cache benchmark | Implemented | v1.10 | `adasdf_benchmark_block_cache` | Compares CPU active block cached query with direct sparse query. |
| Benchmark | block lookup benchmark | Alpha available | v1.17 | `adasdf_benchmark_block_lookup` | Reports linear/hash/Morton lookup timings, cache lookup timings, speedup_vs_linear, mismatch counts, phi-diff fields, fallback counts, and performance-claim gating. |
| Benchmark | contact reduction benchmark | Implemented | v1.13 | `adasdf_benchmark_contact_reduction` | Measures sparse query plus contact stabilization overhead and candidate reduction ratio. |
| Benchmark | CollisionWorld benchmark | Implemented | v1.14 | `adasdf_benchmark_collision_world` | Measures broadphase, sparse world collision, or world contact orchestration timing. |
| Benchmark | CUDA active block cache benchmark | Experimental | v1.11 | `adasdf_benchmark_cuda_block_cache` | Reports CUDA total/kernel/upload/download timing and CPU/direct sparse baselines; CUDA unavailable returns code `20`. |
| Benchmark | hierarchical sampling speed/quality benchmark | Experimental | v1.16 | `adasdf_benchmark_hierarchical_sampling` | Compares exact and hierarchical builds, reports speedup, error, sign mismatch, fallback, guard pass/fail, and alpha.2 diagnostics fields. |
| Benchmark | hierarchical sampling parameter sweep | Experimental | v1.16.0-alpha.2 | `adasdf_sweep_hierarchical_sampling` | Sweeps coarse resolution, transition guard samples, and far-field quality modes; only marks effective acceleration when quality passes and speedup is greater than 1. |
| Benchmark | contact-band sampling benchmark | Alpha available | v1.16.0-alpha.3 | `adasdf_benchmark_contact_band_sampling` | Reports end-to-end timing, core-build timing, marker/audit diagnostics, contact-band quality, coverage, and performance claim gating. |
| Benchmark | contact-band marker sweep | Alpha available | v1.16.0-alpha.3 | `adasdf_sweep_contact_band_sampling` | Sweeps contact-band marker parameters for collision-oriented construction cases. |
| Benchmark | CPU/GPU alignment | Implemented | v1.0 | tests | Optional CUDA skip. |
| Benchmark | real existing-core asset benchmark | Existing-core only | v0.7 | discovered fixtures | Optional. |
| Benchmark | FCL comparison benchmark | Planned | - | - | Not implemented. |
| Benchmark | robot STL benchmark | Planned | - | - | Not implemented. |
