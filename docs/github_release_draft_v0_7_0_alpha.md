# AdaSDF-CL v0.7.0-alpha

First alpha / research-preview release candidate.

## Highlights

- Adaptive SDF builder bridge.
- `.sdfbin` reader/writer round-trip.
- Point signed-distance and gradient query.
- FCL-style `CollisionObject`, `distance`, and `collide` APIs.
- CPU SDF-sampling narrow-phase.
- Symmetric contact generation.
- Deterministic contact reduction.
- CMake install/export support.
- `find_package(AdaSDFCL CONFIG REQUIRED)` downstream integration.
- CLI tools: `adasdf_build`, `adasdf_info`, `adasdf_query`, `adasdf_collide`.

## Validation

- CTest: 17/17 passed.
- Install validation: PASS.
- Alpha validation: PASS.
- Repo clean check: PASS.
- Core-free CI-smoke mode: PASS locally by static workflow review and local
  core-free validation.

## Known Limitations

- Pair collision is approximate CPU SDF-sampling narrow-phase.
- Not FCL ABI-compatible.
- CUDA batched pair query is not implemented.
- Python package is not implemented.
- True surrogate backend is not connected.
- Contact-only `.sdfbin` writer/reader is not implemented.
- Not an industrial-certified collision pipeline.

## Recommended Use

Research preview, algorithm discussion, early integration experiments, and
academic reproduction.

## Not Recommended For

Safety-critical systems, certified industrial contact pipelines, or claims
requiring strict global closest-point guarantees.
