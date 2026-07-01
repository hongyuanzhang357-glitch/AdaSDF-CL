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
- Safe mesh cleanup for near-duplicate vertices, degenerate triangles,
  duplicate triangles, and unused vertices.
- ASCII STL writer for cleaned mesh export.
- Standalone uniform DenseSDF builder for core-free STL-to-SDF construction.
- DenseSDF `.sdfbin` read/write with `ADASDF_DENSE_SDFBIN_V1`.
- `adasdf_build_dense_sdf` CLI and public STL-to-DenseSDF workflow.
- Adaptive octree builder for deterministic near-surface refinement.
- Adaptive block partitioner with one dense block per leaf.
- Adaptive block SDF builder for core-free STL-to-adaptive-block-SDF construction.
- Adaptive block `.sdfbin` read/write with `ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1`.
- `adasdf_build_adaptive_sdf` CLI and public STL-to-adaptive-block-SDF workflow.
- Markdown and JSON-like mesh diagnostics reports.
- Before/after cleanup diagnostics and readiness reports.
- `adasdf_mesh_check` CLI, `adasdf_mesh_clean` CLI, CPU-only mesh diagnostics
  demo, and CPU-only mesh cleanup demo.
- External CMake install/export and downstream example.

## Partial / Experimental

- Surrogate recommender: deterministic demo recommender, not universal or fully
  trained.
- Adaptive builder: core-free adaptive octree/block dense builder is public;
  low-rank compression is not implemented yet.
- Existing-core bridge: available only when the existing research core and
  dependencies are present.
- CUDA expanded query: optional and experimental; no compressed-direct query.
- Block-expanded query: useful for selected/local contact regions.
- Contact manifold: contact reduction exists; stable robot-grade clustering is
  planned.
- Real asset bridge: existing-core fixtures and sampled expansion bridge exist
  when direct query support is available.
- STL import audit: diagnostics, readiness suggestions, and safe cleanup are
  implemented, but no hole filling or self-intersection repair follows from
  the report yet.
- Adaptive compressed builder: v1.6 implements octree/block dense construction.
  Full low-rank compression is not implemented.

## Planned

- Low-rank compression builder for adaptive blocks.
- SVD/Tucker compression.
- Surrogate-guided adaptive build recommendation.
- Complex mesh repair and hole filling.
- Self-intersection detection.
- FCL fallback backend.
- CollisionWorld broadphase.
- CCD.
- Python bindings.
- ROS / MoveIt integration.
- Real robot STL benchmark suite.
- Full low-rank GPU-native SDF query.
