# Sign Mismatch Metrics

AdaSDF-CL v1.1 uses a single sign definition for direct-vs-expanded quality
audits:

```text
inside:    phi < -sign_epsilon
outside:   phi > +sign_epsilon
ambiguous: abs(phi) <= sign_epsilon
```

`direct_sign` is computed from the direct model query. `expanded_sign` is
computed from the expanded query candidate.

## Strict Mismatch

Only inside-vs-outside disagreement counts as strict sign mismatch:

```text
inside  vs outside -> mismatch
outside vs inside  -> mismatch
ambiguous involved -> not a strict mismatch
```

Ambiguous samples are reported separately because tiny distances around zero are
expected to be numerically sensitive.

## Near-Surface Mismatch

A near-surface sample is one whose direct query satisfies:

```text
abs(phi_direct) <= near_surface_band
```

Near-surface sign mismatch is computed over those samples only. This metric is
useful for contact and collision workflows because sign flips close to the
surface are more operationally important than sign differences far away.

## Report Fields

- `sign_mismatch_count`
- `sign_mismatch_rate`
- `ambiguous_sign_count`
- `ambiguous_sign_rate`
- `near_surface_sign_mismatch_count`
- `near_surface_sign_mismatch_rate`
- `fallback_count`
- `fallback_rate`
