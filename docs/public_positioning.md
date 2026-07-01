# Public Positioning

AdaSDF-CL is an FCL-style adaptive SDF collision research library. It is
currently best understood as a complementary SDF backend, not a full FCL
replacement.

## Recommended Wording

- FCL-style SDF collision backend.
- Research-preview adaptive SDF collision library.
- Signed distance, normal, and penetration query.
- Batch query and optional CUDA expanded-SDF path.
- Expanded-SDF quality audit and sign-risk metrics.
- STL mesh diagnostics preflight for import readiness.
- SDF build readiness scoring and repair suggestions.
- Safe mesh cleanup and clean ASCII STL export.
- Standalone uniform DenseSDF builder.
- Standalone adaptive octree/block SDF builder with dense per-block values.
- Adaptive builder preview for future low-rank stages.

## Avoid Wording

- Industrial-grade replacement for FCL.
- Certified collision engine.
- Universal adaptive STL-to-SDF solution.
- Complete adaptive octree/block/low-rank compressed builder.
- Automatic mesh repair tool.
- Automatic hole filling or self-intersection repair tool.
- Industrial mesh certification tool.
- Guaranteed optimal surrogate recommender.
- Complete GPU contact solver.

## Short Description

AdaSDF-CL complements FCL-style workflows by adding SDF-native signed-distance
queries, penetration-depth estimates, contact normals, batch query tooling,
expanded-SDF quality audits, optional CUDA expanded-query paths, STL mesh
diagnostics, SDF build readiness scoring, safe mesh cleanup, and a public
uniform DenseSDF builder, and a public adaptive octree/block SDF builder.
v1.6 adds the first core-free STL-to-adaptive-block-SDF path. Blocks store dense
phi values; low-rank compression is planned for v1.7. It does not fill holes,
repair self-intersections, or claim complete adaptive low-rank SDF construction.
