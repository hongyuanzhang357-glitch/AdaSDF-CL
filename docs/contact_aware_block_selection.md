# Contact-Aware Block Selection

`ActiveBlockSelector` maps sparse samples to a deterministic active block id
set.

Selection is intentionally sparse. It may query phi for the sample points, but
it does not globally expand the model. The selected ids are the seed for local
block expansion.

## Selection Rule

For each sample:

```text
effective_phi = phi - radius
candidate if effective_phi <= threshold + selection_band
```

If the sample is a candidate, the selector adds its containing block. If
`extra_margin` or sample radius is positive, the selector uses a small AABB
around the sample and adds intersecting blocks.

When neighbor inclusion is enabled, blocks adjacent to selected seed blocks are
also included. This helps finite-difference normal queries near block borders.

## Supported Models

- `AdaptiveBlockSDFModel`
- `CompressedAdaptiveBlockSDFModel`

Dense SDF models are queryable, but they have no adaptive block ids and are
therefore not valid inputs for active block selection.
