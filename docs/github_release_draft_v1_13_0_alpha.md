# AdaSDF-CL v1.13.0-alpha

AdaSDF-CL v1.13.0-alpha introduces solver-aware contact candidate
stabilization.

## Added

- `ContactBudget` for hard-contact candidate limits.
- `ContactPatch` and `ContactClusterer`.
- `ContactStabilizer` with duplicate removal, thresholding, clustering, and
  deterministic budget enforcement.
- `SolverContact` and `SolverContactSet`.
- Solver contact CSV, Markdown, and JSON-like export.
- `adasdf_stabilize_contacts`.
- `adasdf_solver_contact_candidates`.
- `adasdf_benchmark_contact_reduction`.
- Python wrapper support for stabilized solver contacts.
- Determinism and contact budget tests.

## Notes

- Solver-ready contacts are not solver impulses.
- v1.13 does not implement a contact solver.
- Contact budget is important for hard-contact dynamics.
- Temporal coherence and warm-start tracking remain planned work.

## Boundaries

This release does not implement PGS, Moreau, LCP, NCP, friction, Jacobian rows,
FCL fallback, CollisionWorld, ROS, MoveIt, CUDA contact solving, or pybind11.
It exports solver-ready candidates, not solver constraints.
