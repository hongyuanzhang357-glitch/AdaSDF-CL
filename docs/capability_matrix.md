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
| Build / Input | adaptive compressed builder interface preview | Experimental | v1.5 | `AdaptiveSDFBuilderPreview`, `adasdf_build_adaptive_sdf_preview` | Dry-run plan only; does not write adaptive compressed `.sdfbin`. |
| Build / Input | standalone adaptive compressed STL builder | Planned | - | - | Full octree/block/low-rank construction remains planned. |
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
| SDF storage / model | existing-core `.sdfbin` read | Existing-core only | v0.2 | `SDFBinReader` | Query requires existing core. |
| SDF storage / model | contact-only `.sdfbin` | Implemented | v0.6 | `ContactOnlySDFBin` | Lightweight contact export format. |
| SDF storage / model | low-rank compressed SDF native public reader | Partial | v0.2 | `CompressedSDF`, `SDFModel` | Full direct public native reader remains limited. |
| Query | point signed distance | Implemented | v0.2 | `adasdf_query`, `SDFModel::sampleDistance` | Direct model query. |
| Query | gradient / normal | Implemented | v0.2 | `SDFModel::sampleGradient`, `sampleNormal` | Analytic or finite-difference backed. |
| Query | CPU direct | Implemented | v1.0.1 | `QueryModeConfig::cpuDirect` | Default stable route. |
| Query | CPU global expanded | Implemented | v1.0.1 | `SDFExpander`, `QueryEngine` | Dense global layout. |
| Query | CPU block expanded | Implemented | v1.0.1 | `BlockSelection`, `QueryEngine` | All or selected blocks. |
| Query | CUDA global expanded | Experimental | v1.0.1 | `CudaResidentExpandedSDF` | Optional CUDA. |
| Query | CUDA block expanded | Experimental | v1.0.1 | `CudaResidentExpandedSDF` | Optional CUDA, best for local points. |
| Query | CUDA compressed-direct | Planned | - | - | Not implemented. |
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
| Collision / Contact | nearest points | Implemented | v0.3 | `DistanceResult` | Pair distance result. |
| Collision / Contact | contact manifold clustering | Partial | v0.5 | `ContactReducer` | Stable robot-grade manifold planned. |
| Collision / Contact | CCD | Planned | - | - | Not implemented. |
| Accuracy / Reliability | expansion quality audit | Implemented | v1.1.0 | `ExpansionQuality`, `adasdf_expansion_quality` | Direct vs expanded. |
| Accuracy / Reliability | max / mean / RMS / p95 error | Implemented | v1.1.0 | quality report, benchmark CSV | Deterministic samples. |
| Accuracy / Reliability | sign mismatch | Implemented | v1.1.0 | `sign_mismatch_rate` | Strict inside/outside mismatch. |
| Accuracy / Reliability | ambiguous sign | Implemented | v1.1.0 | `ambiguous_sign_rate` | Separate from mismatch. |
| Accuracy / Reliability | near-surface sign mismatch | Implemented | v1.1.0 | `near_surface_sign_mismatch_rate` | Contact-risk indicator. |
| Accuracy / Reliability | fallback statistics | Implemented | v1.1.0 | `fallback_count`, `fallback_rate` | Quality and benchmark. |
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
| Integration | Python binding | Planned | - | `PythonBindingPlan` | Plan only. |
| Integration | ROS / MoveIt | Planned | - | - | Not implemented. |
| Integration | FCL fallback backend | Planned | - | - | Not implemented. |
| Integration | FCL adapter | Partial | v0.3 | `FCLAdapter` | API shape only. |
| Benchmark | CPU batch query | Implemented | v1.0 | `adasdf_benchmark_batch_query` | Deterministic. |
| Benchmark | CUDA batch query | Experimental | v1.0 | benchmark CLI | Optional CUDA. |
| Benchmark | warmup / repeat | Implemented | v1.0.2 | `--warmup`, `--repeat` | Repeat statistics. |
| Benchmark | kernel-only / total-time distinction | Implemented | v1.0.2 | CSV timing fields | Clear timing semantics. |
| Benchmark | CPU/GPU alignment | Implemented | v1.0 | tests | Optional CUDA skip. |
| Benchmark | real existing-core asset benchmark | Existing-core only | v0.7 | discovered fixtures | Optional. |
| Benchmark | FCL comparison benchmark | Planned | - | - | Not implemented. |
| Benchmark | robot STL benchmark | Planned | - | - | Not implemented. |
