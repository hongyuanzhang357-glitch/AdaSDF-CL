# Build Recommendation Schema

AdaSDF-CL v1.8.0-alpha exposes build recommendation data through
`include/adasdf/recommendation`.

## Target

`BuildTarget` contains:

- `target_near_surface_error`
- `memory_budget_mb`
- `use_case`: `contact`, `balanced`, `quality`, or `memory-saving`
- `signed_distance`
- `allow_open_unsigned`
- `allow_dense_fallback`
- `prefer_compression`
- `prefer_fast_build`
- `max_build_time_hint_seconds`

## Mesh Features

`MeshFeatureSummary` records vertex/triangle counts, AABB diagonal and volume,
watertightness, boundary/non-manifold/degenerate/duplicate counts, component
count, readiness score, and warnings.

## Recipe

`BuildRecipe` records the recommended path and parameters:

- DenseSDF: dense resolution and padding.
- AdaptiveBlockSDF: min/max octree level, block resolution, target refinement
  error, padding, signed/unsigned mode, and auto-clean.
- CompressedAdaptiveBlockSDF: adaptive parameters plus compression error, rank
  range, fixed-rank flag, low-rank enablement, and near-surface dense fallback.

Each recipe also contains estimated memory, error, compression ratio, build cost
score, feasibility, confidence, warnings, rationale, and a CLI command.

## Report

`BuildRecommendationReport` contains success/error state, target, mesh features,
the selected recipe, candidate recipes, global warnings, limitations, and a
summary.

The JSON output from `BuildRecommendationWriter::toJson` is a compact
JSON-like report intended for inspection and regression tests. It is stable
enough for v1.8 alpha tooling but should not yet be treated as a long-term
wire protocol.
