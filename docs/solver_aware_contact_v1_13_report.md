# AdaSDF-CL v1.13.0-alpha Solver-Aware Contact Report

## Goal

Add solver-aware contact candidate stabilization without implementing a contact
solver.

## New Files

- `include/adasdf/contact/*`
- `src/contact/*`
- `tools/adasdf_stabilize_contacts.cpp`
- `tools/adasdf_solver_contact_candidates.cpp`
- `tools/adasdf_benchmark_contact_reduction.cpp`
- `tests/test_contact_budget.cpp`
- `tests/test_contact_patch.cpp`
- `tests/test_contact_clusterer.cpp`
- `tests/test_contact_stabilizer.cpp`
- `tests/test_solver_contact_builder.cpp`
- `tests/test_solver_contact_exporter.cpp`
- `tests/test_contact_stabilization_report_writer.cpp`
- `tests/test_solver_contact_budget_behavior.cpp`
- `tests/test_contact_determinism.cpp`
- `tests/test_stabilize_contacts_cli.cpp`
- `tests/test_solver_contact_candidates_cli.cpp`
- `tests/test_benchmark_contact_reduction_cli.cpp`
- `docs/solver_aware_contact_candidates.md`
- `docs/contact_budget.md`
- `docs/contact_clustering.md`
- `docs/solver_contact_export.md`
- `docs/contact_reduction_benchmarking.md`
- `docs/github_release_draft_v1_13_0_alpha.md`

## Modified Files

- `CMakeLists.txt`
- `include/adasdf/adasdf.h`
- `include/adasdf/version.h`
- `cmake/adasdf_cl_version.cmake`
- `README.md`
- `CHANGELOG.md`
- `CITATION.cff`
- `docs/alpha_status.md`
- `docs/capability_matrix.md`
- `docs/contact_candidate_api.md`
- `docs/hard_contact_collision_budget.md`
- `docs/implemented_vs_planned.md`
- `docs/public_positioning.md`
- `docs/roadmap.md`
- `docs/sparse_sdf_collision.md`
- `docs/python_cli_wrapper.md`
- `python/adasdf_cli/__init__.py`
- `python/adasdf_cli/parsers.py`
- `python/adasdf_cli/results.py`
- `python/adasdf_cli/tools.py`
- `python/pyproject.toml`
- `python/README.md`
- `scripts/run_alpha_validation.py`
- `scripts/run_install_validation.py`

## Design

`ContactBudget` keeps hard-contact candidate counts small.
`ContactPatch` stores spatially/normal-consistent local patches.
`ContactClusterer` deterministically groups candidates.
`ContactStabilizer` removes duplicates, thresholds penetration, clusters, uses
patch representatives, and enforces the contact budget.
`SolverContactSet` is the exported solver-input layer.

The `stable_key` format is:

```text
object_id:link_id:group_id:patch_id:sample_id
```

## CLI

- `adasdf_stabilize_contacts`: stabilize an existing contact-candidate CSV.
- `adasdf_solver_contact_candidates`: query SDF samples and export stabilized
  solver-ready contacts.
- `adasdf_benchmark_contact_reduction`: benchmark reduction and stabilization
  overhead.

## Python Wrapper

Added:

- `stabilize_contacts`
- `solver_contact_candidates`
- `benchmark_contact_reduction`

## Current Limitations

- Not a contact solver.
- No impulses.
- No friction forces.
- No Jacobian rows.
- No temporal tracking.
- No CollisionWorld or broadphase.
- No FCL hybrid backend.

## Validation

- CPU-only configure: PASS.
- CPU-only build: PASS.
- CPU CTest: PASS, 153/153.
- Python unittest: PASS, 39/39 with real CLI smoke enabled.
- Install validation: PASS.
- Alpha validation: PASS.
- Clean check: PASS.
- CUDA validation: optional for this release; CPU-only paths and CUDA-unavailable
  skip behavior remain supported.

## Next Steps

- Temporal coherence and warm-start contact matching.
- CollisionWorld and broadphase.
- Strict JSON/CSV schema versioning.
- Optional FCL hybrid backend.
