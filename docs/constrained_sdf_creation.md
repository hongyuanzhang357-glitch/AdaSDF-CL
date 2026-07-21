# Constrained SDF Creation

`ConstrainedSDFBuilder` is the v1.18 alpha compiler-preview STL-to-SDF path that
accepts exactly three tuning constraints:

```cpp
adasdf::SDFCreationConstraints constraints;
constraints.max_sdf_file_bytes = 64ull * 1024ull * 1024ull;
constraints.max_decoded_block_bytes = 256ull * 1024ull;
constraints.max_zero_surface_abs_error = 1.0e-3;

adasdf::SDFCreationResult result = adasdf::ConstrainedSDFBuilder::fromSTL(
    "model.stl", "model.sdfbin", constraints);
```

Input and output paths identify the asset. They are not backend tuning
parameters. Octree depth, BVH leaf size, block sizing, compression method,
tolerance, and rank are selected internally and appear in the report.

## Hard constraints

- `max_sdf_file_bytes` covers the complete serialized file, including the
  header, octree, block directory, metadata, compressed payloads, and checksum.
- `max_decoded_block_bytes` covers the largest block output tensor plus the
  required decode workspace/payload residency reported by the selected codec.
- `max_zero_surface_abs_error` is checked only after writing and reloading the
  final candidate asset.

No candidate is returned as feasible when a measured value exceeds its
budget. If no attempted backend parameters satisfy all three constraints, the
result status is `Infeasible`, `model` is null, the final output is absent, and
the report contains failed constraints, attempted parameters, actual values,
and suggested minimum budgets.

## Pipeline

1. Validate the mesh and require watertight signed-distance input.
2. Extract versioned geometry features and obtain deterministic backend advice.
3. Build one triangle BVH and reuse it through an exact signed-phi oracle.
4. Build a padded adaptive octree using sign change, narrow-band distance,
   center interpolation residual, triangle overlap, and normal complexity.
5. Partition blocks from octree subtrees under the decoded-block budget.
6. Reuse exact octree corners and trilinearly interpolate missing coarse nodes.
7. Audit interpolated nodes and promote error/sign failures to exact BVH nodes.
8. Select an adaptive absolute-error-bounded Dense, MatrixSVD, HOSVD, TT, or
   Tucker representation per block. Fall back to Dense when compression is
   larger or violates error/sign guards.
9. Serialize, reload, validate, rewrite measured metadata, reload again, and
   evaluate all three hard constraints.
10. Evaluate all five repair candidates and, among feasible candidates only,
    select lexicographically by complete file bytes, measured build time,
    measured query latency, and deterministic attempt index.

Query latency is measured over the same deterministic `10x10x10` cell-centered
grid inside the candidate AABB. Reports include both `query_sample_count` and
`query_ns_per_sample`, so the cost is not dependent on STL vertex count.

Later repair attempts increase octree depth, split blocks more aggressively,
tighten interpolation promotion and compression absolute-error thresholds, and
raise the adaptive rank ceiling. Reports identify `selected_attempt_index` and
the stable `candidate_cost_order`; heuristic output is never accepted without the
same serialized-reload measurements.

## Error semantics

The current zero-surface result is a `measured estimate`, not a mathematical
strict upper bound. It evaluates the reloaded asset over each STL triangle at
its vertices, edge midpoints, and centroid, then deterministically subdivides
triangles whose phi variation or center interpolation residual is high. Reports
include the unique sample count, maximum terminal sample spacing, and audit
completion flag. The validator also extracts every sign-changing edge zero
crossing from the reloaded block tensors and queries its exact BVH distance to
the STL. The enforced measured value is the maximum of
`mesh_to_sdf_max_abs_phi` and `sdf_to_mesh_max_abs_distance`; both directions
and the zero-crossing sample count are reported. Reports set
`zero_surface_error_is_strict_bound=false` so this scope cannot be confused
with a certified Hausdorff bound. A future strict bound requires conservative
triangle-domain subdivision and interval/Lipschitz certification.

## CLI

```powershell
adasdf_create_sdf model.stl model.sdfbin `
  --max-sdf-file-bytes 67108864 `
  --max-decoded-block-bytes 262144 `
  --max-zero-surface-abs-error 0.001 `
  --report build.json `
  --csv build.csv
```

Exit code `0` means feasible, `3` means structured infeasible, and `2` means
invalid input or a build/I/O failure.

The JSON and CSV outputs include stage timings, exact/interpolated/promoted
node counts, BVH visits/tests, octree and block counts, compression methods and
ranks, byte counts, round-trip status, measured error, and error-bound scope.
Per-attempt exact-query requests, unique queries, cache hits, BVH visits, and
triangle tests cover octree construction, tensor fill, promotion, and final
bidirectional zero-surface validation rather than only the octree stage.
