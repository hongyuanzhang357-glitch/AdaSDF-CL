# AdaSDF-CL v1.0.1-alpha Query Mode And Expansion Report

Date: 2026-06-29

## Summary

AdaSDF-CL v1.0.1-alpha adds an explicit query-mode layer over the existing
batch query API. The release introduces pre-expanded dense SDF layouts and a
CUDA-resident expanded SDF backend while preserving CPU-only builds and the
existing public demo workflow.

## Version

- Version string: `1.0.1-alpha`
- Planned tag: `v1.0.1-alpha`
- Previous public tag retained: `v1.0.0-alpha`

## New Public Surface

- `include/adasdf/query/QueryModeConfig.h`
- `include/adasdf/query/ExpandedSDF.h`
- `include/adasdf/query/SDFExpander.h`
- `include/adasdf/query/QueryEngine.h`
- `include/adasdf/backend/CudaQueryBackend.h` resident expanded SDF API
- `tools/adasdf_query_mode_demo.cpp`

## Mode Availability

| Backend | Expansion | Status | Notes |
| --- | --- | --- | --- |
| CPU | None | Supported | Direct `SDFModel` query |
| CPU | Global | Supported | Pre-expanded global dense grid |
| CPU | Block | Supported | Pre-expanded selected dense blocks |
| CUDA | Global | Supported when CUDA is enabled | Resident global dense grid |
| CUDA | Block | Supported when CUDA is enabled | Resident selected dense blocks |
| CUDA | None | Rejected | Compressed-direct GPU query is not implemented |

Invalid CUDA direct mode reports:

```text
CUDA backend requires pre-expanded SDF data. Use Global or Block expansion.
```

The collision CLI reports:

```text
CUDA collision query requires pre-expanded SDF data.
```

## Implementation Notes

- `QueryModeConfig` validates backend/expansion combinations and block
  selection.
- `SDFExpander` expands the current queryable model into either one global
  dense block or selected dense blocks.
- `ExpandedSDF` provides CPU trilinear sampling and finite-difference normals.
- `QueryEngine` owns preparation, fallback policy, batch query dispatch, and
  stats.
- `CudaResidentExpandedSDF` uploads expanded metadata and values once, launches
  the resident query kernel, and reports device memory and kernel timing.
- `CollisionRequest` and `DistanceRequest` now carry optional query-mode config
  while keeping CPU direct defaults.
- `adasdf_collide` accepts `--backend`, `--expansion`, and `--blocks`.

## Files Added

- `docs/query_modes.md`
- `docs/github_release_draft_v1_0_1_alpha.md`
- `docs/query_mode_and_expansion_v1_0_1_report.md`
- `include/adasdf/query/ExpandedSDF.h`
- `include/adasdf/query/QueryEngine.h`
- `include/adasdf/query/QueryModeConfig.h`
- `include/adasdf/query/SDFExpander.h`
- `src/query/ExpandedSDF.cpp`
- `src/query/QueryEngine.cpp`
- `src/query/QueryModeConfig.cpp`
- `src/query/SDFExpander.cpp`
- `tests/test_block_selection.cpp`
- `tests/test_query_engine_cpu.cpp`
- `tests/test_query_engine_cuda.cpp`
- `tests/test_query_mode_cli.cpp`
- `tests/test_query_mode_config.cpp`
- `tests/test_sdf_expander.cpp`
- `tools/adasdf_query_mode_demo.cpp`

## Files Updated

- `README.md`
- `CHANGELOG.md`
- `CITATION.cff`
- `CMakeLists.txt`
- `cmake/adasdf_cl_version.cmake`
- `docs/alpha_status.md`
- `docs/api_design.md`
- `docs/benchmarking.md`
- `docs/code_reading_guide_for_beginners.md`
- `docs/cpu_narrowphase_audit.md`
- `docs/external_integration.md`
- `docs/fcl_compatibility.md`
- `docs/github_publication_checklist.md`
- `docs/github_publication_commands.md`
- `docs/gpu_backend.md`
- `docs/limitations.md`
- `docs/original_ui_cuda_audit.md`
- `docs/roadmap.md`
- `docs/upload_package_manifest.md`
- `docs/v1_cuda_validation_against_original_ui.md`
- `include/adasdf/adasdf.h`
- `include/adasdf/backend/CudaQueryBackend.h`
- `include/adasdf/config.h`
- `include/adasdf/query/CollisionRequest.h`
- `include/adasdf/query/DistanceRequest.h`
- `include/adasdf/version.h`
- `reports/alpha_validation_summary.md`
- `reports/install_validation_summary.md`
- `scripts/run_alpha_validation.py`
- `src/backend/CudaQueryBackend.cpp`
- `src/backend/CudaQueryKernels.cu`
- `src/query/QueryAPI.cpp`
- `tools/adasdf_collide.cpp`
- `benchmarks/adasdf_benchmark_batch_query.cpp`

## Validation

| Check | Result |
| --- | --- |
| CPU Release build | PASS |
| CPU CTest | 37/37 PASS |
| CUDA Release build | PASS through ASCII-only source/build path |
| CUDA CTest | 37/37 PASS |
| CPU query-mode demo | PASS |
| CUDA query-mode demo | PASS |
| `CUDA + None` rejection | PASS |
| Benchmark CSV selected-block quoting | PASS; `0,1,2` is quoted |

## Benchmark Snapshot

Local Release benchmark rows:

| Backend | Expansion | Blocks | Points | Setup ms | Kernel ms | Total ms | ns/query | Expanded MB | GPU MB | Max phi err | Max normal err |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| CPU | None | all | 100,000 | 0.0008 | NA | 5.0764 | 50.764 | 0 | 0 | 0 | 0 |
| CPU | Global | all | 100,000 | 4.5588 | NA | 17.9953 | 179.953 | 2.0001 | 0 | 0.005406 | 0.873865 |
| CPU | Block | all | 100,000 | 3.4537 | NA | 19.8335 | 198.335 | 1.5006 | 0 | 0.011475 | 2 |
| CUDA | Global | all | 100,000 | 5.8217 | 1.0820 | 5.2922 | 52.922 | 2.0001 | 2.0001 | 0.005406 | 0.873865 |
| CUDA | Block | all | 100,000 | 5.43 | 7.7973 | 12.1534 | 121.534 | 1.5006 | 1.5005 | 0.011475 | 2 |
| CUDA | Block | 0,1,2 | 100,000 | 2.7632 | 3.5393 | 7.3181 | 73.181 | 0.7503 | 0.7502 | 0.011624 | 0.903103 |
| CUDA | Global | all | 1,000,000 | 5.8224 | 10.4934 | 40.7194 | 40.7194 | 2.0001 | 2.0001 | 0.006410 | 0.899688 |
| CUDA | Block | 0,1,2 | 1,000,000 | 2.6606 | 30.8537 | 59.8738 | 59.8738 | 0.7503 | 0.7502 | 0.011744 | 0.916844 |

Expanded-grid errors are expected because the candidate output is sampled from
finite-resolution dense grids. Normals are finite-difference approximations and
are not certified contact normals.

## Boundaries

- No old tags are moved.
- No force push is required.
- Generated `.sdfbin`, SVG, build directories, install directories, and raw
  benchmark logs remain outside the repository.
- CUDA compressed-direct query remains unsupported.
- Full low-rank compressed SDF GPU expansion remains future work.
- CUDA pair collision remains future work.
- This release does not claim an industrial GPU contact solver.

## Next Pre-Release Recommendation

Use `v1.1.0-alpha` for the next feature pre-release if it adds public dense-grid
quality controls, sign-mismatch metrics, existing adaptive asset expansion, or
GPU pair-collision work. Use `v1.0.2-alpha` only for narrow documentation,
packaging, or compatibility fixes on top of the v1.0.1 API.
