# Sparse Collision v1.9 Report

## Goal

AdaSDF-CL v1.9.0-alpha adds CPU-only sparse SDF collision query and contact
candidate APIs without adding FCL, existing-core, Python, or CUDA dependencies
to the C++ sparse module.

## New Files

- `include/adasdf/sparse/CollisionSampleSet.h`
- `include/adasdf/sparse/CollisionSampleSetIO.h`
- `include/adasdf/sparse/SparseSDFQuery.h`
- `include/adasdf/sparse/SparseCollisionQuery.h`
- `include/adasdf/sparse/ContactCandidateReducer.h`
- `include/adasdf/sparse/SparseQueryReportWriter.h`
- `src/sparse/*.cpp`
- `tools/adasdf_sparse_query.cpp`
- `tools/adasdf_sparse_collide.cpp`
- `tools/adasdf_contact_candidates.cpp`
- `tools/adasdf_benchmark_sparse_query.cpp`
- `tests/data/samples/*.csv`
- `tests/test_*sparse*.cpp`
- `docs/sparse_sdf_collision.md`
- `docs/contact_candidate_api.md`
- `docs/hard_contact_collision_budget.md`
- `docs/sparse_query_benchmarking.md`

## Design

`CollisionSampleSet` stores sparse query samples with optional radius, object
ids, link ids, group ids, weight, and label. CSV I/O supports header and
headerless files, comments, blank lines, defaults, and clear parse errors.

`SparseSDFQuery` evaluates `phi = model.sampleDistance(position)` and computes
`effective_phi = phi - radius` when sample radius is enabled. A sample collides
when `effective_phi <= threshold`. The default path is phi-only and does not
compute normals.

`SparseCollisionQuery` provides collision-only, clearance, and candidate-search
modes. Collision-only defaults to early exit. Clearance and candidate-search
force full traversal.

`ContactCandidateReducer` filters by threshold, sorts deterministically by
effective phi and sample id, applies optional reduction radius, and keeps Top-K
candidates.

## CLI And Python

The v1.9 CLIs are `adasdf_sparse_query`, `adasdf_sparse_collide`,
`adasdf_contact_candidates`, and `adasdf_benchmark_sparse_query`.

Python wrappers were added as `sparse_query`, `sparse_collide`,
`contact_candidates`, and `benchmark_sparse_query`. The wrapper treats
`adasdf_sparse_collide` return code `10` as successful collision detection.

## Boundaries

This release does not implement FCL fallback, `CollisionWorld`, CCD, full
contact manifold clustering, solver constraints, GPU block cache, or
GPU-native compressed direct query.

Direct compressed query is appropriate for sparse queries, debugging, fallback,
and small point sets. It is not the main high-throughput GPU strategy.

Runtime memory saving for compressed SDF should primarily come from
contact-aware active block expansion/cache, planned for v1.10.

## Validation Results

Final validation was run before tagging:

- CPU configure/build: PASS.
- CPU CTest: PASS, 111/111 tests.
- Python unittest: PASS, 28/28 tests with real CLI smoke and sparse samples.
- Install validation: PASS, including installed sparse CLIs and Python wrappers.
- Alpha validation: PASS with install validation.
- CUDA-specific CTest: not run because `nvcc` was not found on PATH; alpha
  validation configured with `ADASDF_CL_ENABLE_CUDA=ON`, detected no CUDA
  compiler, built without CUDA kernels, and passed the graceful fallback path.
- Clean check: PASS.

## Next Steps

- v1.10: contact-aware active block expansion/cache.
- v1.11: CUDA active block cache baseline.
- v1.12: BVH-accelerated builder.
- Future: solver-aware contact manifold and `CollisionWorld` broadphase.
