# Expanded SDF Accuracy

ExpandedSDF is an approximation of direct `SDFModel` queries. It trades memory
for repeat query speed by sampling a model into dense global or block grids and
then querying those grids with trilinear interpolation.

## Controls

- `global_resolution`: dense grid resolution for global expansion.
- `block_resolution`: dense grid resolution for each expanded block.
- `padding`: extra domain margin around the model or block bounds.
- `near_surface_band`: direct-query distance band used for near-surface risk
  metrics.
- `sign_epsilon`: inside/outside/ambiguous sign threshold.
- `clamp_outside_expanded_domain`: clamp domain misses to the nearest expanded
  domain boundary.
- `allow_direct_fallback_outside`: let callers treat domain misses as direct
  query fallback opportunities.

Higher resolution usually reduces interpolation error but increases expanded
memory. Global expansion is best for global batch queries. Block expansion is
best for local contact regions and selected blocks.

## Quality Audit

Use:

```bash
adasdf_expansion_quality cube_adaptive.sdfbin --expansion global --global-resolution 64 --samples 10000
adasdf_expansion_quality cube_adaptive.sdfbin --expansion block --blocks all --block-resolution 32 --samples 10000
```

The audit compares expanded queries against the direct model query and reports:

- max, mean, RMS, and p95 absolute error;
- strict sign mismatch rate;
- ambiguous sign rate;
- near-surface sign mismatch rate;
- fallback count and fallback rate;
- worst sampled point.

Sign mismatch is especially important for contact workflows because an
inside/outside flip near the surface can change contact generation decisions.
Near-surface sign mismatch is therefore a more sensitive risk indicator than
global mean error.

## Boundary

v1.1 expands models by sampling into global or block dense layouts. Existing-core
models can use the same sampled bridge when they expose direct query support.
This is not full low-rank compressed SDF GPU-native query.
