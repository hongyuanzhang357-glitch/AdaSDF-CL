# AdaSDF-CL v1.9.0-alpha

AdaSDF-CL v1.9.0-alpha introduces sparse point-to-SDF collision queries and
contact candidate extraction.

## Added

- `CollisionSampleSet` and CSV I/O.
- Sparse point-to-SDF query API.
- Collision-only, clearance, and candidate-search sparse query modes.
- Sample radius support through `effective_phi = phi - radius`.
- Collision-only early-exit checks.
- Top-K contact candidate reduction with optional `reduction_radius`.
- CSV, Markdown, and JSON-like sparse query reports.
- `adasdf_sparse_query`.
- `adasdf_sparse_collide`.
- `adasdf_contact_candidates`.
- `adasdf_benchmark_sparse_query`.
- Pure-Python `adasdf_cli` wrappers for sparse query, collision, candidates,
  and sparse benchmarking.

## Notes

- Sparse query defaults to phi-only and does not compute normals unless
  requested.
- `adasdf_sparse_collide` return code `10` means collision detected; it is not
  a program failure.
- Contact candidates are reduced candidates, not full solver contacts and not a
  complete contact manifold.
- Sparse point collision is a first-class use case for robot links, foot
  contacts, grippers, tools, sampled geometry, and hard-contact simulators.
- Direct compressed query is useful for sparse queries, debugging, fallback,
  and small point sets, but it is not the main high-throughput GPU path.
- Runtime memory saving for compressed SDF should primarily come from
  contact-aware active block expansion/cache, planned for v1.10.

## Validation

- CPU CTest target: 111/111 PASS.
- Python wrapper unittest target: 28/28 PASS.
- Install validation target: PASS.
- Alpha validation target: PASS with install validation.
- CUDA-specific validation: not run because `nvcc` was not found on PATH; the
  CUDA-requested configure path gracefully built without CUDA kernels.
- Clean check target: PASS.
