# AdaSDF-CL v0.7.0-alpha.1

CI-fixed alpha / research-preview release candidate.

This pre-release supersedes the earlier `v0.7.0-alpha` preflight tag by including the Ubuntu install validation portability fix.

## Highlights

- Adaptive SDF builder bridge
- `.sdfbin` reader/writer round-trip
- Point signed-distance and gradient query
- FCL-style `CollisionObject`, `distance`, and `collide` APIs
- CPU SDF-sampling narrow-phase
- Symmetric contact generation
- Deterministic contact reduction
- CMake install/export support
- `find_package(AdaSDFCL CONFIG REQUIRED)` downstream integration
- CLI tools: `adasdf_build`, `adasdf_info`, `adasdf_query`, `adasdf_collide`

## Fixes in alpha.1

- Fixed Ubuntu install validation portability.
- Added robust downstream executable discovery for Linux single-config and Windows multi-config generators.
- Improved CI log visibility for install validation failures.

## Validation

- Ubuntu Release CI: PASS
- Windows Release CI: PASS
- CTest: 17/17 passed
- Install validation: PASS
- Alpha validation: PASS
- Repo clean check: PASS

## Known limitations

- Pair collision is approximate CPU SDF-sampling narrow-phase
- Not FCL ABI-compatible
- CUDA batched pair query is not implemented
- Python package is not implemented
- True surrogate backend is not connected
- Contact-only sdfbin writer/reader is not implemented
- Not an industrial-certified collision pipeline

## Recommended use

Research preview, algorithm discussion, early integration experiments, and academic reproduction.

## Not recommended for

Safety-critical systems, certified industrial contact pipelines, or claims requiring strict global closest-point guarantees.
