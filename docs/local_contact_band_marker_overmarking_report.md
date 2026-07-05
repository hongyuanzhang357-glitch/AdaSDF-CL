# AdaSDF-CL Local Contact-Band Marker Over-Marking Report

Local branch: `perf/contact-focused-narrow-band-local`

This is a local performance sprint report. It is not a public release note.
The public version string remains `1.16.0-alpha.2`; no public tag is created or
moved by this work.

## Goal

The previous contact-band prototype proved that collision-focused exact
sampling can speed up a cube fixture, but it over-marked the wavy sphere so
heavily that almost every block stayed on the exact path. This sprint reduces
that over-marking without changing the `.sdfbin` format or the default public
sampling behavior.

Implemented locally:

- `conservative-aabb`, `distance-aware`, and `hybrid` marker modes;
- box-triangle distance refinement for contact-band candidates;
- local halo marking to avoid whole block-boundary halo expansion;
- marker diagnostics for candidate, rejected, marked, halo, and timing counts;
- a contact-band marker sweep CLI.

## Previous Contact-Band Baseline

The earlier local contact-band benchmark used conservative triangle-AABB
marking. It preserved contact-band quality, but the wavy sphere was the failure
boundary.

| Case | Contact blocks | Far-field blocks | Exact node ratio | Speedup | Quality | Effective speedup |
| --- | ---: | ---: | ---: | ---: | --- | --- |
| cube L5 contact-band | 5048 | 24080 | 0.161813 | 1.45272 | PASS | yes |
| cube L5 thin-band | 5048 | 24080 | 0.148922 | 1.48630 | PASS | yes |
| wavy L4 contact-band | 3872 | 210 | 0.942491 | 0.813745 | PASS | no |
| wavy L4 thin-band | 3872 | 210 | 0.941802 | 0.823123 | PASS | no |

The wavy sphere had `3872 / 4082` blocks classified as contact-band blocks.
That left only about 5.8% exact-node reduction, so marker overhead, coarse fill,
and quality audit cost outweighed the saved exact sampling.

## Why The Wavy Sphere Over-Marked

The conservative marker expands every fine-cell AABB by the contact band and
accepts any triangle AABB overlap. On dense or curved meshes, many triangle AABBs
cross many fine-cell AABBs even when the actual triangle is still farther than
the intended contact band. The method is safe, but it treats broadphase overlap
as contact evidence.

The wavy sphere is a good stress case for this because local curvature and many
small triangle AABBs produce a large number of conservative overlaps. Once a
block contains any contact-band nodes, the previous global block halo could also
promote extra boundary nodes to the exact path.

## Marker Modes

| Mode | Broadphase | Refinement | Intended use |
| --- | --- | --- | --- |
| `conservative-aabb` | expanded fine-cell AABB vs triangle AABB | none | reference-safe legacy marker |
| `distance-aware` | same conservative candidate set | box-triangle distance acceptance | reduce false contact cells |
| `hybrid` | same conservative candidate set | distance acceptance with slightly wider adaptive band | more conservative distance-aware option |

The distance-aware and hybrid modes do not broaden the candidate set beyond the
legacy conservative contact-band width. They only refine the candidate cells
that conservative AABB marking would already consider, so the new modes are
designed to reduce over-marking rather than create a new wider marker.

## Fixed Benchmark Results

Common settings:

- block resolution: `8`;
- CPU-only local build;
- `--sampling contact-band`;
- contact-band quality audit enabled;
- normal audit enabled;
- local halo enabled for distance-aware and hybrid runs.

| Case | Marker | Contact blocks | Far-field blocks | Exact node ratio | Speedup | Quality | Effective speedup |
| --- | --- | ---: | ---: | ---: | ---: | --- | --- |
| cube L5 distance-aware | distance-aware | 5048 | 24080 | 0.0866263 | 2.34612 | PASS | yes |
| wavy L4 distance-aware | distance-aware | 922 | 3160 | 0.122179 | 1.36031 | PASS | yes |
| wavy L4 hybrid | hybrid | 994 | 3088 | 0.137497 | 1.28865 | PASS | yes |

The cube still has speedup greater than 1. The wavy sphere now also has speedup
greater than 1, which was the key failure in the previous local sprint.

## Marker Diagnostics

| Case | Candidate overlaps | Distance refined cells | Distance rejected cells | Marked cells | Local halo nodes | Global halo nodes | Overmark estimate | Marker ms | Refinement ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| cube L5 distance-aware | 368457 | 368457 | 121201 | 247256 | 1291904 | 0 | 0.328942 | 3325.69 | 80.0169 |
| wavy L4 distance-aware | 1982248 | 1982248 | 1898990 | 83258 | 255352 | 0 | 0.957998 | 858.214 | 409.565 |
| wavy L4 hybrid | 1969321 | 1969321 | 1864939 | 104382 | 287367 | 0 | 0.946996 | 916.91 | 441.446 |

The wavy sphere still creates many broadphase candidates, but distance
refinement rejects about 95% of them before they become exact nodes. Local halo
keeps `global_halo_node_count` at zero for these local experiments.

## Quality Results

| Case | Max abs phi error | P95 phi error | Sign mismatches | Near-surface sign mismatches | P95 normal error deg | Normal flips | Quality |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| cube L5 distance-aware | 6.07153e-18 | 4.98733e-18 | 0 | 0 | 1.20742e-06 | 0 | PASS |
| wavy L4 distance-aware | 3.33067e-16 | 1.8735e-16 | 0 | 0 | 1.70755e-06 | 0 | PASS |
| wavy L4 hybrid | 3.53884e-16 | 2.15106e-16 | 0 | 0 | 1.70755e-06 | 0 | PASS |

The reduced marker did not introduce contact-band phi, sign, or normal failures
on the current cube and wavy-sphere fixtures.

## Sweep Summary

The local sweep tested 192 marker/width/halo/far-field combinations.

| Best case | Marker | Width | Halo layers | Cell-size factor | Safety factor | Far-field resolution | Speedup | Exact node ratio | Quality |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| best speedup | distance-aware | 0.001 | 0 | 0.25 | 0.5 | 2 | 1.4947 | 0.107262 | PASS |
| best exact-node ratio | distance-aware | 0.0005 | 0 | 0.25 | 0.5 | 2 | 1.45188 | 0.106753 | PASS |

The sweep confirms that distance-aware marking with a smaller marker cell-size
factor is the strongest local option so far. It reduces contact blocks from the
previous wavy L4 `3872` to the `857-861` range while keeping the contact-band
quality gate green.

## Interpretation

The primary performance problem was not the contact-band sampler itself. It was
the marker promoting too many curved-surface cells and blocks into the exact
path. Distance-aware refinement attacks that directly:

- wavy L4 contact-band blocks dropped from `3872` to `922` in the fixed
  distance-aware benchmark;
- wavy L4 exact-node ratio dropped from `0.942491` to `0.122179`;
- wavy L4 speedup improved from `0.813745` to `1.36031`;
- the sweep found a quality-passing `1.4947x` wavy L4 case at exact-node ratio
  `0.107262`.

The marker cost is now visible, especially on the wavy fixture. This is expected
because the broadphase still produces about two million candidate overlaps and
the distance-aware path refines each candidate. The important change is that the
refinement cost is now buying a much smaller exact path.

## Recommendation

This local result is strong enough to prepare a public `v1.16.0-alpha.3`
candidate after release hardening, provided the public release wording stays
precise:

- claim contact-band over-marking reduction on the tested cube and wavy-sphere
  fixtures;
- keep the path explicit and default-off unless release hardening decides
  otherwise;
- report speedup only when `contact_band_quality_passed` is true and measured
  speedup is greater than 1;
- avoid a universal speedup claim until thin gaps, robot-like CAD, and denser
  curved meshes have coverage.

If not promoted immediately, the next bottleneck is marker cost: the current
distance-aware marker still refines many candidate cells. A spatially tighter
triangle-cell candidate stage, candidate caching per block, or batched
box-triangle distance evaluation would be the next optimization target.

## Artifacts

Local benchmark artifacts were written under the sprint `WORKROOT` temporary
directory, outside the repository:

- `benchmarks/local_marker_cube_L5_distance.csv`;
- `benchmarks/local_marker_wavy_L4_distance.csv`;
- `benchmarks/local_marker_wavy_L4_hybrid.csv`;
- `benchmarks/local_marker_wavy_L4_sweep.csv`;
- `reports/local_marker_cube_L5_distance.md`;
- `reports/local_marker_wavy_L4_distance.md`;
- `reports/local_marker_wavy_L4_hybrid.md`;
- `reports/local_marker_wavy_L4_sweep.md`.
