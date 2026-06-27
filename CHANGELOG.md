# Changelog

## 0.6.0-alpha

Initial alpha / research-preview release candidate.

### Added

- Adaptive SDF builder bridge.
- `.sdfbin` reader/writer round-trip.
- `SDFModel` point distance and gradient query.
- FCL-style `CollisionObject`, `collide`, and `distance` APIs.
- CPU SDF-sampling narrow-phase backend.
- Symmetric contact generation.
- Deterministic contact reduction.
- `adasdf_build` CLI.
- Validation card documentation.
- Repository clean check script.
- Alpha validation script.
- GitHub CI and issue/PR templates.
- CMake install/export package skeleton.

### Known Limitations

- Pair collision is an approximate SDF-sampling narrow-phase.
- CUDA batched pair query is not implemented.
- FCL ABI compatibility is not provided.
- Surrogate recommender is an API placeholder unless a backend is explicitly connected.
- Contact-only `.sdfbin` remains planned.
