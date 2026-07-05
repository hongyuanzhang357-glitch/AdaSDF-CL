# AdaSDF-CL Local Contact-Band Sampling Report

Local build id: `contact-focused-narrow-band-local`

This is a local performance sprint report, not a public release draft. The
public version string, public tags, and GitHub release notes remain unchanged
at `1.16.0-alpha.2`.

## Goal

This sprint tests whether AdaSDF-CL can reduce construction work by sampling
only contact-focused narrow-band nodes exactly, while using relaxed far-field
interpolation outside the collision-relevant band.

The experiment deliberately changes the benchmark question:

- old question: can global phi quality be preserved while reducing samples?
- new question: can contact-band phi, sign, and normals be preserved while
  reducing far-field exact samples?

## Why Contact Band

Robot collision detection mainly depends on the zero surface and the narrow
region around it. Far-field phi values are useful for coarse rejection, but
they usually do not need the same accuracy as near-contact nodes. A global
quality guard can therefore spend most of its time protecting values that are
not decisive for contact generation.

The contact-band path keeps exact BVH sampling where the surface may affect
contacts and relaxes the rest.

## Implementation Summary

Implemented locally:

- triangle-driven `ContactBandMarker`;
- `ContactBandSamplingPolicy` with explicit default-off enable flag;
- `ContactBandBlockSampler` using exact nodes plus coarse far-field fill;
- contact-band-only phi, sign, and normal audit;
- builder CLI integration through `--sampling contact-band`;
- `adasdf_benchmark_contact_band_sampling`;
- five CPU-only tests for marker, policy, sampler, audit, and benchmark CLI.

The path does not change `.sdfbin` format, does not require CUDA, does not
require FCL, and does not require the existing core.

## Benchmark Setup

Common settings:

- block resolution: `8`
- threads: `2`
- target error: `1e-3`
- contact-band layers: `1`
- halo exact layers: `1`
- far-field resolution: `3`
- far-field mode: `coarse-interpolate`
- far-field sign policy: reuse coarse sign
- normal audit: enabled
- normal limit: `5.0 deg`

Fixtures:

- `tests/data/mesh_diagnostics/closed_cube_ascii.stl`, max level `5`;
- `tests/data/mesh_diagnostics/wavy_sphere_ascii.stl`, max level `4`.

## Benchmark Results

| Case | Exact reference ms | Contact-band ms | Speedup | Quality passed | Effective speedup allowed |
| --- | ---: | ---: | ---: | --- | --- |
| cube L5 contact-band | 18024.4 | 12407.3 | 1.45272 | true | true |
| cube L5 thin-band | 17940.5 | 12070.5 | 1.48630 | true | true |
| wavy L4 contact-band | 784.976 | 964.647 | 0.813745 | true | false |
| wavy L4 thin-band | 780.611 | 948.353 | 0.823123 | true | false |

## Sampling Counts

| Case | Total blocks | Contact blocks | Far-field blocks | Total nodes | Exact nodes | Predicted nodes | Far-field nodes | Coarse samples |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| cube L5 contact-band | 29128 | 5048 | 24080 | 14913536 | 2413208 | 12500328 | 12500328 | 786456 |
| cube L5 thin-band | 29128 | 5048 | 24080 | 14913536 | 2220960 | 12692576 | 12692576 | 786456 |
| wavy L4 contact-band | 4082 | 3872 | 210 | 2089984 | 1969792 | 120192 | 120192 | 110214 |
| wavy L4 thin-band | 4082 | 3872 | 210 | 2089984 | 1968352 | 121632 | 121632 | 110214 |

## Reduction Metrics

| Case | Exact node ratio | Exact sample reduction ratio | Distance queries | Sign queries | Sign query reduction ratio |
| --- | ---: | ---: | ---: | ---: | ---: |
| cube L5 contact-band | 0.161813 | 0.838187 | 3199664 | 3199664 | 0.785452 |
| cube L5 thin-band | 0.148922 | 0.851078 | 3007416 | 3007416 | 0.798343 |
| wavy L4 contact-band | 0.942491 | 0.0575086 | 2080006 | 2080006 | 0.0047742 |
| wavy L4 thin-band | 0.941802 | 0.0581976 | 2078566 | 2078566 | 0.0054632 |

## Contact-Band Quality

| Case | Max abs error | RMS error | P95 error | Sign mismatches | Near-surface sign mismatches |
| --- | ---: | ---: | ---: | ---: | ---: |
| cube L5 contact-band | 6.07153e-18 | 2.12411e-18 | 4.98733e-18 | 0 | 0 |
| cube L5 thin-band | 6.07153e-18 | 2.45596e-18 | 4.98733e-18 | 0 | 0 |
| wavy L4 contact-band | 3.53884e-16 | 3.58403e-17 | 2.22045e-16 | 0 | 0 |
| wavy L4 thin-band | 3.53884e-16 | 3.58607e-17 | 2.22045e-16 | 0 | 0 |

## Normal Audit

| Case | Mean normal deg | P95 normal deg | Max normal deg | Normal flips | Near-surface normal flips |
| --- | ---: | ---: | ---: | ---: | ---: |
| cube L5 contact-band | 2.44176e-09 | 1.20742e-06 | 1.47878e-06 | 0 | 0 |
| cube L5 thin-band | 4.78892e-11 | 1.20742e-06 | 1.47878e-06 | 0 | 0 |
| wavy L4 contact-band | 9.68351e-08 | 1.20742e-06 | 1.70755e-06 | 0 | 0 |
| wavy L4 thin-band | 9.67167e-08 | 1.20742e-06 | 1.70755e-06 | 0 | 0 |

## Interpretation

The cube fixture demonstrates the intended result. Only about 15-16% of fine
nodes need exact BVH sampling, contact-band quality passes, and speedup is
greater than 1. The thinner band reduces exact nodes further and improves
speedup from `1.45272` to `1.48630`.

The wavy sphere fixture is the important failure boundary. Conservative
triangle-AABB marking classifies 3872 of 4082 blocks as contact-band blocks.
That leaves only about 5.8% sample reduction, so the marker, mask, coarse fill,
and audit overhead make the contact-band path slower than the exact reference.
Quality still passes, but no effective speedup claim is allowed.

## Recommendation

Do not promote this local sprint directly as a broad public speedup claim yet.
The cube result is strong enough to justify continued work, but the wavy result
shows that conservative contact-band marking can over-mark complex surfaces.

Recommended next step for a possible `v1.16.0-alpha.3`:

- keep the current default-off CLI path;
- add a parameter sweep for contact-band width, layers, and halo policy;
- add a less over-conservative marker option for curved/wavy meshes;
- add more fixtures with thin gaps and robot-like geometry;
- report effective speedup only when `speedup > 1` and contact-band quality
  passes.

## Limitations

- Contact-band width requires tuning.
- Thin features and small gaps need additional validation.
- Far-field phi is relaxed and is not globally exact.
- This path is aimed at collision detection, not full-domain high-accuracy SDF
  reconstruction.
- Current wavy L4 results do not support a universal speedup claim.
