# AdaSDF-CL v1.8.0-alpha

AdaSDF-CL v1.8.0-alpha adds surrogate-guided build recommendation for public
core-free STL workflows.

## Highlights

- Added `adasdf_recommend_build`.
- Added C++ recommendation APIs under `adasdf/recommendation`.
- Added deterministic mesh-feature-based estimates for DenseSDF,
  AdaptiveBlockSDF, and CompressedAdaptiveBlockSDF.
- Added Markdown and JSON-like recommendation reports.
- Added optional key-value surrogate profiles.
- Added recommendation examples, validation coverage, and docs.

## Boundary

This release provides an experimental deterministic build recommender. It is
not a universal trained model, not fully trained, and not an optimality
guarantee.

The recommendation step does not build by default and does not write `.sdfbin`
files. Users should run the emitted command only after reviewing warnings,
confidence, and candidate estimates, then validate the final SDF with build,
compression, and quality reports.

## Planned

- trained model integration;
- online calibration;
- full low-rank compressed SDF GPU-native query;
- pybind11 bindings;
- FCL fallback;
- CollisionWorld broadphase.
