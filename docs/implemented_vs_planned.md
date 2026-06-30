# Implemented vs Planned

## Implemented Now

- Core-free demo SDF workflow.
- Demo surrogate recommender.
- Demo adaptive `.sdfbin`.
- Point signed-distance, gradient, and normal query.
- FCL-style pair collision API.
- Contact point, normal, penetration depth, and signed distance.
- Contact reduction and max contact cap.
- SVG collision visualization.
- CPU/CUDA query modes over direct or expanded layouts.
- Global and block expansion.
- Block selection.
- Benchmark timing semantics with warmup, repeat, kernel-only, and total-time
  fields.
- ExpandedSDF quality audit.
- Sign mismatch, ambiguous sign, and near-surface mismatch metrics.
- ASCII and binary STL reader for diagnostics.
- STL mesh diagnostics preflight for AABB, scale, watertight, boundary,
  non-manifold, duplicate, degenerate, component, and isolated-vertex checks.
- Mesh readiness scoring, severity classification, and repair suggestions for
  SDF build preflight.
- Markdown and JSON-like mesh diagnostics reports.
- `adasdf_mesh_check` CLI and CPU-only mesh diagnostics demo.
- External CMake install/export and downstream example.

## Partial / Experimental

- Surrogate recommender: deterministic demo recommender, not universal or fully
  trained.
- Adaptive builder: core-free demo adaptive builder is public; full arbitrary
  STL builder needs existing core or future public work.
- Existing-core bridge: available only when the existing research core and
  dependencies are present.
- CUDA expanded query: optional and experimental; no compressed-direct query.
- Block-expanded query: useful for selected/local contact regions.
- Contact manifold: contact reduction exists; stable robot-grade clustering is
  planned.
- Real asset bridge: existing-core fixtures and sampled expansion bridge exist
  when direct query support is available.
- STL import audit: diagnostics and readiness suggestions are implemented, but
  no automatic repair or standalone arbitrary-STL SDF build follows from the
  report yet.

## Planned

- Standalone arbitrary STL adaptive builder.
- Mesh repair.
- Self-intersection detection.
- FCL fallback backend.
- CollisionWorld broadphase.
- CCD.
- Python bindings.
- ROS / MoveIt integration.
- Real robot STL benchmark suite.
- Full low-rank GPU-native SDF query.
