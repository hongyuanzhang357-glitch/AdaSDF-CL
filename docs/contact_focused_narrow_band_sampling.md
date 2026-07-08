# Contact-Focused Narrow-Band SDF Sampling

Release: `v1.16.0-alpha.3`

v1.17.1-alpha keeps this sampling algorithm unchanged and adds JSON benchmark
contract output plus build profile/progress reporting around the public build
CLIs, including compressed SDF builder workflows.

This document describes the alpha contact-focused narrow-band sampling path.
The path is enabled only when the user explicitly selects
`--sampling contact-band`; the default builder path remains exact.

## Motivation

The v1.16 hierarchical sampling path protected global SDF quality. That is a
good default for full-field reconstruction, but it can spend too much time
protecting far-field phi values that are not usually the limiting factor in
robot collision detection.

For collision detection, the primary requirements are concentrated near the
zero surface:

- accurate zero-surface placement;
- accurate phi inside a contact band;
- stable sign inside the contact band;
- stable normals inside the contact band;
- cheap and conservative far-field rejection.

The contact-focused path therefore changes the quality target from global
quality guard plus fallback to contact-band exact sampling plus relaxed
far-field interpolation.

## Components

The alpha.3 release adds these default-off components:

- `ContactBandMarker`
- `ContactBandSamplingPolicy`
- `ContactBandBlockSampler`
- `ContactBandQualityAudit`
- `ContactBandDiagnostics`
- `ContactBandReportWriter`
- `adasdf_benchmark_contact_band_sampling`
- `adasdf_sweep_contact_band_sampling`

The implementation does not change the `.sdfbin` format and does not require
CUDA, FCL, or the existing core. It targets collision-oriented construction,
not globally high-accuracy full-field SDF reconstruction.

## Marker

`ContactBandMarker` works from triangle geometry rather than from a predicted
field:

1. Build the block-local fine cell AABB.
2. Expand the cell AABB by `contact_band_width`.
3. Query triangle AABB overlap through `TriangleBVH`.
4. Mark the cell corner nodes as `contact_band_node` if overlap is found.
5. Expand contact nodes by `contact_band_layers` into `exact_required`.
6. Mark block halo layers as exact for blocks that contain contact-band nodes.
7. Treat blocks with no contact-band nodes as far-field blocks.

The current overlap test is conservative triangle-AABB overlap. It is intended
to avoid missing true zero-surface neighborhoods, even if that over-marks some
nodes on dense or highly curved meshes.

## Sampling Policy

For blocks with contact-band nodes:

- `exact_required` nodes are sampled with `BVHSDFSampler`;
- non-exact nodes are filled from a coarse grid;
- contact-band quality is audited against an exact reference;
- far-field error is not used as the main gate.

For blocks without contact-band nodes:

- only a coarse grid is sampled;
- fine nodes are filled by interpolation or far-field policy;
- the block is counted as far-field;
- no global strict fallback is run.

The default far-field mode is `coarse-interpolate`. The CLI also accepts
`constant-sign` and `clamped-distance`.

## Quality Gate

The contact-band quality gate is intentionally local:

```text
contact_band_quality_passed =
    contact_band_p95_error <= limit
    && contact_band_sign_mismatch_count == 0
    && near_surface_sign_mismatch_count == 0
    && p95_normal_angle_error_deg <= normal_limit
    && near_surface_normal_flip_count == 0
```

Far-field phi error can still be inspected as a sanity check, but it is not the
main failure condition for this collision-focused path.

## CLI

Builder CLIs now accept:

```text
--sampling exact|hierarchical|contact-band
--contact-band-width value
--contact-band-layers N
--halo-exact-layers N
--far-field-resolution N
--far-field-mode coarse-interpolate|constant-sign|clamped-distance
--contact-band-marker conservative-aabb|distance-aware|hybrid
--marker-cell-size-factor value
--marker-safety-factor value
--marker-min-band value
--marker-max-band value
--disable-global-halo
--local-halo-only
--reuse-far-field-sign
--no-reuse-far-field-sign
--audit contact-band|global
--normal-audit
--contact-band-normal-audit
--coverage-audit
--coverage-samples-per-axis N
--contact-band-normal-error-limit-deg value
```

The default remains `--sampling exact`. `--timing-mode` belongs to
`adasdf_benchmark_contact_band_sampling`, not to builder CLIs.

## Benchmark Metrics

`adasdf_benchmark_contact_band_sampling` reports:

- `exact_reference_wall_time_ms`;
- `contact_band_wall_time_ms`;
- `speedup_end_to_end`;
- `speedup_core_build`;
- diagnostic speedups such as `speedup_excluding_marker`;
- block counts;
- exact, predicted, far-field, and coarse sample counts;
- exact-node and sample-reduction ratios;
- distance and sign query counts;
- contact-band phi/sign/normal errors;
- `contact_band_quality_passed`;
- `coverage_passed`;
- `performance_claim_allowed`.

The default benchmark timing mode is `end-to-end`: marker and required audit
work are included in the contact-band wall time. `speedup_excluding_marker` is
diagnostic only and must not be used as a release headline metric.

An external performance claim is allowed only when:

```text
speedup_end_to_end > 1
&& contact_band_quality_passed == true
&& coverage_passed == true
&& contact_band_sign_mismatch_count == 0
&& near_surface_sign_mismatch_count == 0
&& marker is included in speedup
```

See `docs/contact_band_timing_semantics.md` for the full timing vocabulary.

## Current Limitations

- Contact-band width and layer count remain parameter-sensitive.
- Thin features and small gaps need additional mesh coverage before public
  speedup claims.
- Far-field phi is relaxed and is not guaranteed to be globally exact.
- The path is intended for collision-focused SDF construction, not global
  high-accuracy SDF reconstruction.
- Conservative triangle-AABB marking can over-mark complex surfaces, which
  reduces speedup when most blocks become contact-band blocks.
- External wording should describe collision-oriented/contact-band construction
  acceleration, not global SDF reconstruction acceleration.
