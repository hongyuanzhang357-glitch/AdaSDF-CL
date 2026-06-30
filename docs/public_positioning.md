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

## Avoid Wording

- Industrial-grade replacement for FCL.
- Certified collision engine.
- Universal STL-to-SDF solution.
- Automatic mesh repair tool.
- Guaranteed optimal surrogate recommender.
- Complete GPU contact solver.

## Short Description

AdaSDF-CL complements FCL-style workflows by adding SDF-native signed-distance
queries, penetration-depth estimates, contact normals, batch query tooling,
expanded-SDF quality audits, and optional CUDA expanded-query paths.
v1.2 adds STL mesh diagnostics as an import preflight step; it does not yet
claim full arbitrary-STL adaptive SDF construction.
