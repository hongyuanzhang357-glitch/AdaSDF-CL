# AdaSDF-CL

Adaptive Signed Distance Field Collision Library

Status: 1.0.3-alpha / research preview
Build system: CMake
License: MIT
Tests: CTest

AdaSDF-CL is an alpha collision and contact library built around signed distance fields. It provides an FCL-style API for distance, collision, and contact queries while keeping CUDA, FCL, Python, and full adaptive backend work optional or future-facing.

v1.0.3-alpha keeps the v1.0.2 CUDA benchmark semantics and resident query workspace, and adds explicit output modes, CUDA device-only benchmark timing, reusable output-buffer paths, workspace reporting, and a block lookup fallback fast path. CUDA is not required; CPU-only builds remain fully usable.

The original `v1.0.2-alpha` and `v1.0.2-alpha.1` tags are retained for traceability. The recommended public pre-release is `v1.0.3-alpha`.

## v1.0.3-alpha Quick Start

```bash
git clone https://github.com/hongyuanzhang357-glitch/AdaSDF-CL.git
cd AdaSDF-CL
git checkout v1.0.3-alpha

cmake -S . -B build -DADASDF_CL_BUILD_EXAMPLES=ON -DADASDF_CL_BUILD_TESTS=ON -DADASDF_CL_BUILD_BENCHMARKS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release --prefix install

install/bin/adasdf_recommend_demo --shape box --target-error 1e-3 --memory-mb 64 --block-memory-mb 16 --top-k 5
install/bin/adasdf_build_demo_adaptive cube_adaptive.sdfbin --shape box --target-error 1e-3 --memory-mb 64 --block-memory-mb 16 --use-surrogate
install/bin/adasdf_info cube_adaptive.sdfbin
install/bin/adasdf_query cube_adaptive.sdfbin --point 0 0 0
install/bin/adasdf_collide_boxes_demo --target-error 1e-3 --memory-mb 64 --offset 0.25 0 0 --max-contacts 8 --view collision.svg
install/bin/adasdf_query_mode_demo --backend cpu --expansion none --points 100000
install/bin/adasdf_query_mode_demo --backend cuda --expansion global --points 100000
install/bin/adasdf_benchmark_batch_query --points 1000000 --query-backend cuda --expansion global --output phi --kernel-only --reuse-resident --warmup 10 --repeat 50 --out benchmark.csv
```

On Windows, installed tools usually have `.exe` suffixes, for example `install/bin/adasdf_recommend_demo.exe`.

Generated `.sdfbin` and `.svg` files should stay in build, install, or temporary directories and should not be committed to the source tree.

## What Works Now

- Core-free analytic box SDF model.
- Demo `.sdfbin` format `ADASDF_DEMO_SDFBIN_V1`.
- Demo adaptive `.sdfbin` format `ADASDF_DEMO_ADAPTIVE_SDFBIN_V1`.
- Demo surrogate recommender that maps target error and memory constraints to adaptive demo parameters.
- Demo adaptive builder that creates octree metadata, block metadata, and rank metadata for an analytic box backend.
- `adasdf_recommend_demo`, `adasdf_build_demo_adaptive`, `adasdf_collide_boxes_demo`, `adasdf_make_demo_box`, `adasdf_info`, `adasdf_query`, and `adasdf_collide`.
- SVG collision visualization with box outlines, contact points, normal arrows, backend/method labels, minimum distance, and contact counts.
- Installable CMake package consumed with `find_package(AdaSDFCL CONFIG REQUIRED)`.
- CPU batch query API for signed distance, gradient, and normal queries.
- Query-mode configuration for CPU direct, CPU global-expanded, CPU block-expanded, CUDA global-expanded, and CUDA block-expanded queries.
- `ExpandedSDF`, `SDFExpander`, and `QueryEngine` for reusable pre-expanded query preparation.
- Optional CUDA resident expanded SDF backend for analytic/demo adaptive box SDFs.
- CUDA query workspace reuse for points, phi, and normal buffers when `--reuse-resident` is enabled.
- CUDA phi-only expanded SDF kernel for signed-distance-only benchmark comparison.
- Explicit benchmark output modes: `--output phi` and `--output phi,normal`.
- CUDA device-only benchmark mode for no-download GPU-side timing.
- Deterministic benchmark point generation and `adasdf_benchmark_batch_query` with backend, expansion, block selection, memory, setup, timing breakdown, warmup/repeat statistics, kernel-only mode, workspace reuse fields, block lookup fields, and error columns.

## Backend Boundary

AdaSDF-CL currently has two practical modes.

### Core-Free Public Demo

The public repository can be configured, built, tested, installed, and consumed by external CMake projects without the original research core.

v0.9 supports a public workflow:

```text
target error + memory constraints
-> demo surrogate recommendation
-> demo adaptive SDF metadata
-> demo adaptive .sdfbin
-> point query
-> two-box collision
-> SVG collision view
```

The demo adaptive builder uses analytic box SDF queries and demo adaptive metadata to exercise the full public workflow. It is not the full adaptive compressed STL-to-SDF builder.

### Existing-Core Enhanced Build

When configured with `ADASDF_CL_USE_EXISTING_CORE=ON` and a valid `ADASDF_CL_EXISTING_ROOT`, AdaSDF-CL can use the existing research core to build and read existing-core adaptive `.sdfbin` assets. That path remains separate from the v0.9 demo backend.

## Build From Source

CMake options:

- `ADASDF_CL_BUILD_EXAMPLES`: build example executables.
- `ADASDF_CL_BUILD_TESTS`: build and register CTest tests.
- `ADASDF_CL_BUILD_BENCHMARKS`: build benchmark tools, default `ON`.
- `ADASDF_CL_ENABLE_DEMO_BACKEND`: enable the core-free demo backend, default `ON`.
- `ADASDF_CL_ENABLE_DEMO_SURROGATE`: enable the demo surrogate recommender, default `ON`.
- `ADASDF_CL_ENABLE_COLLISION_VIEWER`: enable SVG collision visualization, default `ON`.
- `ADASDF_CL_USE_EXISTING_CORE`: use the existing OctreeBlockLowRankSDF core when available.
- `ADASDF_CL_ENABLE_ADAPTIVE_BUILDER`: enable the public adaptive builder API.
- `ADASDF_CL_ENABLE_SURROGATE_RECOMMENDER`: enable the placeholder DASH-SDF recommender API.
- `ADASDF_CL_ENABLE_CUDA`: enable the optional CUDA batch query backend, default `OFF`.

CMake configure summary reports:

```text
CUDA backend: ON/OFF
CUDA toolkit: found/not found
Benchmarks: ON/OFF
```

## CPU/GPU Batch-Query Benchmark

```bash
adasdf_benchmark_batch_query --points 10000,100000,1000000 --query-backend cpu --expansion none --out cpu_direct.csv
adasdf_benchmark_batch_query --points 10000,100000,1000000 --query-backend cuda --expansion global --out cuda_global.csv
adasdf_benchmark_batch_query --points 10000,100000,1000000 --query-backend cuda --expansion block --blocks 0,1,2 --out cuda_blocks.csv
adasdf_benchmark_batch_query --points 1000000 --query-backend cuda --expansion global --output phi --kernel-only --reuse-resident --warmup 10 --repeat 50 --out cuda_global_phi_kernel.csv
adasdf_benchmark_batch_query --points 1000000 --query-backend cuda --expansion global --output phi,normal --reuse-resident --warmup 10 --repeat 50 --out cuda_global_full.csv
adasdf_benchmark_batch_query --points 1000000 --query-backend cuda --expansion global --output phi --device-only --kernel-only --reuse-resident --warmup 10 --repeat 50 --out cuda_global_phi_device_only.csv
```

The CUDA backend is optional. If CUDA is not available, AdaSDF-CL remains fully usable in CPU mode and CUDA benchmark rows are marked as skipped.

```text
CUDA backend unavailable
```

v1.0.3-alpha supports CUDA batch queries over pre-expanded global or block dense SDF data for the core-free analytic/demo adaptive box backend. Full low-rank compressed SDF GPU expansion is planned but not yet complete.

### Benchmark timing semantics

`total_ms` and `kernel_ms` intentionally measure different scopes. `kernel_ms` is CUDA event time for the expanded SDF kernel only. `total_ms` includes host-side output allocation, point upload, kernel synchronization, result download, CPU postprocess, and one-shot allocation/free when `--reuse-resident` is not used. Original UI CUDA numbers that report warmed kernel average time should be compared to `--kernel-only --output phi --reuse-resident` rows, not to full `total_ms` rows.

`--output phi` computes signed distance only. `--output phi,normal` computes signed distance plus finite-difference normals and is the full query primitive currently used by contact workflows. `--device-only` / `--no-download` skips D2H result download and correctness checks; it is a performance-analysis mode, not a correctness mode. The legacy `--phi-only` flag is retained as an alias for `--output phi`.

Benchmark output fields are:

```text
query_backend,expansion_mode,selected_blocks,num_points,expanded_memory_mb,gpu_resident_memory_mb,setup_ms,expand_ms,upload_sdf_ms,allocation_ms,h2d_points_ms,kernel_ms,sync_ms,d2h_results_ms,postprocess_ms,free_ms,total_ms,query_kernel_ms,query_total_ms,ns_per_query,queries_per_second,fallback_count,max_abs_phi_error,max_normal_error,cuda_available,warmup,repeat,kernel_min_ms,kernel_mean_ms,kernel_max_ms,kernel_std_ms,total_min_ms,total_mean_ms,total_max_ms,total_std_ms,output_mode,phi_only,reuse_resident,kernel_only,workspace_reused,allocation_count,workspace_capacity,workspace_device_memory_mb,block_lookup_count,block_scan_count,center_block_hit_rate,neighbor_same_block_rate,download_results,correctness_checked,host_memory,layout,status,error_message
```

## Query Modes

The public query-mode API separates execution backend from expansion strategy:

```cpp
auto build = adasdf::DemoAdaptiveSDFBuilder::build(build_request);

adasdf::ExpansionOptions expansion;
expansion.expansion = adasdf::QueryExpansionMode::Global;
expansion.global_resolution = 64;

adasdf::QueryEngine engine(
    build.model,
    adasdf::QueryModeConfig::cudaGlobalExpanded(),
    expansion);
engine.prepare();

adasdf::BatchQueryOutput output = engine.queryBatch(points);
```

Supported mode combinations:

| Backend | Expansion | Status |
| --- | --- | --- |
| CPU | None | Direct `SDFModel` query |
| CPU | Global | Pre-expanded dense global grid |
| CPU | Block | Pre-expanded dense selected blocks |
| CUDA | Global | GPU-resident dense global grid |
| CUDA | Block | GPU-resident dense selected blocks |
| CUDA | None | Invalid: CUDA requires pre-expanded SDF data |

## Demo Adaptive SDFBin Format

v0.9 adds a text demo adaptive format:

```text
ADASDF_DEMO_ADAPTIVE_SDFBIN_V1
shape box
center 0 0 0
half_extent 0.5 0.5 0.5
unit m
target_near_surface_error 0.001
memory_limit_mb 64
block_expand_limit_mb 16
octree_nodes ...
blocks ...
surrogate demo_v0_9
warning demo_surrogate_not_universal_not_fully_trained
```

`SDFBinReader::read()` detects v0.9 adaptive demo magic first, then v0.8 demo magic, then falls back to the existing-core reader.

## C++ Sketch

```cpp
#include <adasdf/adasdf.h>

adasdf::DemoAdaptiveBuildRequest build_request;
build_request.use_surrogate = true;
build_request.target_near_surface_error = 1.0e-3;
build_request.memory_limit_mb = 64.0;

auto build = adasdf::DemoAdaptiveSDFBuilder::build(build_request);

adasdf::CollisionObject a(build.model);
adasdf::CollisionObject b(build.model);
b.setTransform(adasdf::Transform::fromTranslation({0.25, 0.0, 0.0}));

adasdf::CollisionRequest request;
request.enable_contact = true;
request.max_contacts = 8;

adasdf::CollisionResult result;
adasdf::collide(a, b, request, result);
```

`CollisionRequest::max_contacts`, `adasdf_collide --max-contacts`, and `adasdf_collide_boxes_demo --max-contacts` all cap the returned contacts. Output includes both requested and returned counts.

## Limitations

- The v0.9 surrogate is a demo / smoke / initial recommender only.
- It is not universal, not fully trained, and not an optimality guarantee.
- It is not a reliable parameter selector for arbitrary STL inputs.
- The demo adaptive builder supports analytic boxes, not arbitrary meshes.
- Full adaptive STL-to-compressed-SDF construction remains future work for the public standalone backend.
- Pair collision is an approximate SDF-sampling narrow-phase.
- CUDA support is limited to optional batch signed-distance/normal queries over pre-expanded global/block dense data for analytic/demo adaptive boxes.
- `CUDA + None` is intentionally rejected because CUDA compressed-direct query is not implemented.
- Selected-block CUDA benchmarks are intended for local contact-region point distributions; global uniform point clouds can make selected blocks slower or less representative.
- CUDA pair-collision acceleration, full low-rank compressed SDF GPU expansion, Python bindings, and FCL ABI compatibility are not implemented.

See `docs/limitations.md`.

## Docs

- `docs/demo_surrogate_recommender.md`
- `docs/demo_adaptive_builder.md`
- `docs/collision_visualization.md`
- `docs/query_modes.md`
- `docs/benchmarking.md`
- `docs/gpu_backend.md`
- `docs/core_free_demo_backend.md`
- `docs/alpha_status.md`
- `docs/query_mode_and_expansion_v1_0_1_report.md`
- `docs/cuda_benchmark_semantics_v1_0_2_report.md`
- `docs/cuda_performance_optimization_v1_0_3_report.md`
- `docs/github_release_draft_v1_0_2_alpha.md`
- `docs/github_release_draft_v1_0_2_alpha_1.md`
- `docs/github_release_draft_v1_0_3_alpha.md`

## Citation

See `CITATION.cff`. The alpha citation metadata is a placeholder and does not claim a DOI.

## License

MIT. See `LICENSE`.
