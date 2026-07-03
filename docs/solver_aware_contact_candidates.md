# Solver-Aware Contact Candidates

AdaSDF-CL v1.13.0-alpha adds a solver-ready contact candidate layer on top of
the v1.9 sparse collision and Top-K candidate API. It turns many penetrating
SDF samples into a small, deterministic set of contacts that external
hard-contact solvers can consume.

This is not a contact solver. It does not compute impulses, friction forces,
Jacobian rows, LCP/NCP constraints, or solver residuals.

## Pipeline

```text
sparse SDF penetrating samples
  -> ContactCandidateReducer
  -> duplicate removal
  -> penetration thresholding
  -> spatial/normal patch clustering
  -> patch representatives
  -> contact budget
  -> SolverContactSet
```

## CLI

```bash
adasdf_solver_contact_candidates model.sdfbin samples.csv \
  --threshold 1e-3 \
  --top-k 32 \
  --max-contacts 8 \
  --patch-radius 0.02 \
  --with-normal \
  --out solver_contacts.csv \
  --report solver_contacts.md

adasdf_stabilize_contacts candidates.csv \
  --max-contacts 8 \
  --patch-radius 0.02 \
  --out solver_contacts.csv \
  --report stabilized.md
```

## Boundaries

- Solver-ready contacts are not solver impulses.
- `SolverContactSet` is an input layer for external hard-contact solvers.
- More contact points can increase solver cost.
- Contact budget is a first-class control.
- Patch clustering reduces redundant nearby samples.
- Normal consistency avoids mixed-normal patches.
- Deterministic ordering supports reproducibility.
- `stable_key` is for warm-start or temporal matching by external code.
- v1.13 does not implement temporal tracking.
- Full solver, friction, Jacobian, constraints, and impulses are out of scope.
