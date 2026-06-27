# AdaSDF-CL v0.5 CPU Narrow-Phase Report

## Goal

The v0.5 goal was to upgrade the old pair-query fallback from an API-validation path into a named CPU research-preview narrow-phase with better candidate sampling, symmetric SDF queries, normal stabilization, penetration filtering, contact reduction, statistics, examples, and tests.

This round intentionally did not add CUDA, Python bindings, a real FCL adapter, direct `sdf::PairCollisionSDF`, or GitHub release work.

## Completed Work

- Added `CpuNarrowPhase` as the named CPU SDF-sampling pair-query backend.
- Moved pair fallback logic out of `QueryAPI.cpp`.
- Added `CandidatePointSampler` for transformed world-space candidate points.
- Added `ContactGenerator` for symmetric SDF queries and raw contact generation.
- Added `ContactReducer` for deterministic contact reduction and `max_contacts` enforcement.
- Added result statistics for candidate points, raw contacts, reduced contacts, backend info, and method info.
- Improved approximate distance behavior for separated and overlapping AABBs.
- Updated pair collision and FCL-style examples to print narrow-phase statistics.
- Added `adasdf_contact_reduction_demo`.
- Added four CPU-only tests.
- Updated docs and limitations.

## New Files

- `include/adasdf/query/CandidatePointSampler.h`
- `include/adasdf/query/CpuNarrowPhase.h`
- `include/adasdf/query/ContactGenerator.h`
- `include/adasdf/query/ContactReducer.h`
- `src/query/CandidatePointSampler.cpp`
- `src/query/CpuNarrowPhase.cpp`
- `src/query/ContactGenerator.cpp`
- `src/query/ContactReducer.cpp`
- `examples/07_contact_reduction_demo.cpp`
- `tests/test_candidate_point_sampler.cpp`
- `tests/test_contact_generator.cpp`
- `tests/test_contact_reducer.cpp`
- `tests/test_cpu_narrowphase.cpp`
- `docs/cpu_narrowphase_audit.md`
- `docs/limitations.md`

## Modified Files

- `include/adasdf/adasdf.h`
- `include/adasdf/query/CollisionRequest.h`
- `include/adasdf/query/CollisionResult.h`
- `include/adasdf/query/DistanceResult.h`
- `src/query/CollisionResult.cpp`
- `src/query/DistanceResult.cpp`
- `src/query/QueryAPI.cpp`
- `examples/03_collision_between_two_objects.cpp`
- `examples/05_fcl_style_api.cpp`
- `CMakeLists.txt`
- `README.md`
- `CITATION.cff`
- `docs/api_design.md`
- `docs/fcl_compatibility.md`
- `docs/roadmap.md`

## Current Query Flow

```text
CollisionObject A/B
  -> QueryAPI backend selection
  -> ExistingPairCollisionBridge if available
  -> CpuNarrowPhase otherwise
  -> world AABB broadphase
  -> CandidatePointSampler
  -> ContactGenerator symmetric SDF query
  -> normal orientation and stabilization
  -> signed-distance / penetration filtering
  -> ContactReducer
  -> CollisionResult / DistanceResult
```

## Candidate Sampling Strategy

`CandidatePointSampler` currently uses the source object's model AABB:

- AABB corners.
- AABB face centers.
- AABB center.
- AABB grid points filtered by source-model near-surface SDF value.

The returned points are world coordinates. Sampling respects the object's transform. The near-surface band uses the request band plus a relative fallback based on AABB extent and grid resolution, so small absolute defaults do not starve cube-like fixtures.

## Symmetric SDF Query Convention

For collision, v0.5 samples points from A against B and, by default, points from B against A. Each world point is transformed into the target model's local coordinates before SDF query.

`CollisionRequest::symmetric_query=false` keeps a one-way A against B path for experiments, but the default is symmetric.

## Normal Direction Convention

Contact normals are reported in world coordinates from object B toward object A. Target SDF gradients are transformed with rotation only. After symmetric generation, normals are stabilized against the center direction from B to A to avoid mixed directions in one pair query. If the SDF gradient is near zero, the fallback normal uses target-center or object-center direction.

## Contact Reduction Strategy

`ContactReducer`:

- sorts raw contacts by penetration depth from deep to shallow;
- uses signed distance and point coordinates as deterministic tie-breakers;
- merges contacts whose positions are close and whose normal cosine is above the threshold;
- normalizes kept normals;
- clamps penetration depth to non-negative values;
- caps output by `CollisionRequest::max_contacts`.

This is a deterministic heuristic, not a manifold optimizer.

## Distance Definition

If world AABBs are separated, `distance()` returns the AABB lower-bound distance and approximate nearest AABB points.

If world AABBs overlap, `distance()` returns the best signed value found by symmetric candidate-point SDF sampling. The nearest points are approximated from the sampled point, signed distance, and normal.

This is an approximate SDF-sampling distance, not a strict global closest-distance solve.

## Difference from v0.3 Fallback

v0.3:

- monolithic logic inside `QueryAPI.cpp`;
- fixed AABB/block-center candidate list;
- immediate contact append until `max_contacts`;
- no deterministic contact reduction;
- less explicit normal convention;
- fewer result statistics.

v0.5:

- named `CpuNarrowPhase`;
- separate sampler, generator, and reducer modules;
- symmetric SDF query by default;
- stabilized B-to-A contact normals;
- deterministic contact reduction;
- query-mode-aware sampling defaults;
- detailed result statistics.

## Test Results

Validation command:

```bash
ctest --test-dir build/adasdf_cl-v0_5 -C Debug --output-on-failure
```

Result:

- 17/17 tests passed.
- New tests passed:
  - `test_candidate_point_sampler`
  - `test_contact_generator`
  - `test_contact_reducer`
  - `test_cpu_narrowphase`

## Example Results

Cube build:

- Build status: success.
- AABB min: `-0.025 -0.025 -0.025`.
- AABB max: `1.025 1.025 1.025`.
- memory footprint: `10432 bytes`.
- reload validation: success.

Pair collision example with small offset:

- backend: `CPU narrow-phase: symmetric SDF-sampling research preview`.
- method: `symmetric candidate-point SDF query + deterministic contact reduction`.
- colliding: true.
- minimum distance: about `-0.489924`.
- candidate points: `606`.
- raw contacts: `2`.
- reduced contacts: `2`.
- SDF queries: `4242`.

Contact reduction demo:

- separated offset: collide false, distance `0.2`, 0 SDF queries.
- near-contact offset: collide true, distance about `-0.025005`, raw contacts `74`, reduced contacts `4`.
- penetrating offset: collide true, distance about `-0.249894`, raw contacts `76`, reduced contacts `4`.
- penetrating normals are stabilized in the object B to object A direction.

## Clean Repo Check

Validation command:

```bash
python adasdf_cl/scripts/check_repo_clean.py adasdf_cl
```

Result: PASS.

## Current Limitations

- CPU narrow-phase is approximate and sampling-dependent.
- `distance()` is not a strict global closest-distance solver.
- Contact reduction is heuristic and deterministic, not a full contact manifold optimizer.
- CUDA batched narrow-phase is not implemented.
- FCL ABI compatibility is not provided.
- Direct `sdf::PairCollisionSDF` remains unavailable.
- Python bindings are not implemented.
- Contact-only `.sdfbin` writer/reader remains future work.

## Next Round Recommendation

The suggested next stage is:

```text
AdaSDF-CL v0.6: GitHub alpha release hardening
```

Recommended focus:

- repository initialization or publication preparation;
- public README polish;
- CI;
- release notes;
- version and citation checks;
- license review;
- local path hygiene;
- alpha tag preparation.
