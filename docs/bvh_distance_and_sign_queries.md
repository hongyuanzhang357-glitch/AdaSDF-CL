# BVH Distance And Sign Queries

v1.12.0-alpha introduces small CPU query primitives used by the builder
sampler.

## Distance

`BVHNearestTriangleQuery` traverses TriangleBVH nodes, prunes by AABB squared
distance, and tests only leaf triangles that can improve the current nearest
distance. Ties use the lowest triangle id to keep results deterministic.

The reference brute path still uses `pointTriangleDistance` over every valid
triangle. The BVH path does not change point-triangle distance math; it only
reduces the candidate set.

## Sign

`BVHRayIntersectionQuery` counts unique ray/triangle intersections with a
deterministic positive-x ray. Hits at shared edges or vertices are marked
ambiguous. The SDF sampler can fall back to `MeshSign::classifyPoint` for
ambiguous sign samples so closed cube and boundary-adjacent cases retain the
reference behavior.

## Boundaries

- The sign query is still an alpha-level ray-casting classifier.
- It is not a full robust mesh winding-number implementation.
- It does not repair open meshes.
- Signed builds can still require watertight meshes.
