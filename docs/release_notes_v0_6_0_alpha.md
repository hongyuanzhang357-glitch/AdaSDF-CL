# AdaSDF-CL 0.6.0-alpha Release Notes

AdaSDF-CL 0.6.0-alpha is a research-preview release candidate for adaptive compressed SDF collision experiments.

## Suitable For

- Building small STL-derived adaptive SDF assets when the existing core is available.
- Saving and reloading `.sdfbin` assets.
- Point signed-distance and finite-difference gradient queries.
- FCL-style object/request/result API experiments.
- CPU approximate pair collision with symmetric SDF sampling and deterministic contact reduction.
- Reproducing the included cube validation path.

## Not Suitable For

- Industrial-certified collision guarantees.
- ABI-compatible replacement of FCL.
- CUDA batched pair queries.
- Global closest-distance certification.
- Universal surrogate recommendations.
- Large benchmark claims.

## Quick Validation Summary

The local v0.6 validation path builds examples and tests, runs CTest, runs repository hygiene checks, builds a cube `.sdfbin`, queries it, runs pair collision, and runs the contact reduction demo.

## Tests

- 17 CTest tests are expected in the current alpha validation build.
- Tests that require the existing core skip gracefully when it is unavailable.

## Examples

Working examples:

- load `.sdfbin` and query signed distance.
- collide two objects through the CPU narrow-phase.
- FCL-style API wrapper.
- build then query.
- contact reduction demo.

Preview examples:

- adaptive builder API sketch.
- CUDA batch query sketch.

## Known Limitations

- Pair collision is approximate and sampling-dependent.
- Distance over overlapping AABBs is approximate.
- Contact reduction is heuristic.
- CUDA, real FCL ABI integration, Python package, and contact-only writer/reader are not implemented.
- DASH-SDF runtime recommender is a placeholder.

## Next Roadmap

v0.7 should focus on external integration and packaging:

- `find_package(AdaSDFCL CONFIG REQUIRED)` validation.
- downstream CMake example project.
- install tree tests.
- package config tests.
- public API naming cleanup.
