# AdaSDF-CL v1.6.0-alpha

AdaSDF-CL v1.6.0-alpha introduces the first public adaptive octree/block SDF
builder. It generates block-wise dense SDF data from STL using near-surface
refinement, but does not yet apply low-rank compression.

## Highlights

- Added `AdaptiveOctree` and `AdaptiveOctreeBuilder`.
- Added `AdaptiveBlockPartitioner`.
- Added `AdaptiveBlockSDFBuilder` and `AdaptiveBlockSDFModel`.
- Added `ADASDF_ADAPTIVE_BLOCK_SDFBIN_V1` read/write support.
- Added `adasdf_build_adaptive_sdf`.
- Added adaptive block build reports.
- Added adaptive block query, collision, roundtrip, CLI, and dense-comparison tests.
- Updated the adaptive builder preview to mark octree/block construction as implemented and low-rank compression as planned.

## Boundaries

- Block data is stored as dense per-block phi values.
- Low-rank compression is not implemented in v1.6.0-alpha.
- SVD/Tucker compression is planned for v1.7.0-alpha.
- Surrogate-guided recommendation is planned for v1.8.0-alpha.
- GPU-native compressed adaptive query remains planned work.

## Validation

- CPU-only configure/build/test: PASS, 82/82 tests.
- Install validation: PASS.
- Alpha validation: PASS.
- Clean repo check: PASS.
- CUDA configure/build/test: PASS via an ASCII `subst` path, 82/82 tests.
- CUDA remains an optional query-backend feature over expanded layouts.
