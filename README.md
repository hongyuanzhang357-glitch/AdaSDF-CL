# AdaSDF-CL

Adaptive Signed Distance Field Collision Library

Status: 0.6.0-alpha / research preview  
Build system: CMake  
License: MIT  
Tests: CTest

AdaSDF-CL is an alpha collision and contact library built around adaptive compressed signed distance fields. It provides an FCL-style API for SDF-based distance, collision, and contact queries while keeping CUDA, FCL, Python, and surrogate-model integrations optional.

## What Is AdaSDF-CL?

AdaSDF-CL loads and builds compact adaptive SDF models, saves and reloads `.sdfbin` assets, evaluates signed distance and gradients, and runs a CPU SDF-sampling narrow-phase for pair collision research experiments.

It is intended for research-preview workflows in robotics, simulation, contact modeling, and SDF collision experiments. It is not a certified industrial collision pipeline and is not an ABI-compatible drop-in replacement for FCL.

## Core Features

- Adaptive SDF builder bridge for STL input through the existing OctreeBlockLowRankSDF core.
- `.sdfbin` reader and writer round-trip for existing-core-backed models.
- `SDFModel` point signed-distance, finite-difference gradient, and normal queries.
- FCL-style `CollisionObject`, `CollisionRequest`, `CollisionResult`, `DistanceRequest`, and `DistanceResult`.
- CPU approximate SDF-sampling narrow-phase with symmetric candidate queries.
- Deterministic contact reduction controlled by `CollisionRequest::max_contacts`.
- Command-line `adasdf_build` tool.
- Validation-card documentation for the DASH-SDF surrogate work, with runtime recommender kept as a placeholder.

## Current Status

Working:

- Build an adaptive SDF from STL when the existing core and dependencies are available.
- Save and reload `.sdfbin`.
- Query point signed distance and gradient.
- Run FCL-style pair collision through `CpuNarrowPhase`.
- Run examples and tests without CUDA, FCL, Python, UI, or large datasets.
- Run repository hygiene checks.

API preview / planned:

- OBJ and in-memory mesh builder bridge.
- Contact-only `.sdfbin` writer/reader.
- CUDA batched pair query.
- FCL ABI-compatible adapter.
- Python package.
- Real DASH-SDF surrogate backend.
- Strict global closest-distance solver.

## Quick Start

```bash
cmake -S . -B build -DADASDF_CL_BUILD_EXAMPLES=ON -DADASDF_CL_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

For local development from the parent workspace, the v0.6 validation build used:

```bash
cmake -S adasdf_cl -B build/adasdf_cl-v0_6_alpha -DADASDF_CL_BUILD_EXAMPLES=ON -DADASDF_CL_BUILD_TESTS=ON -DADASDF_CL_USE_EXISTING_CORE=ON -DADASDF_CL_ENABLE_ADAPTIVE_BUILDER=ON -DADASDF_CL_ENABLE_SURROGATE_RECOMMENDER=ON
cmake --build build/adasdf_cl-v0_6_alpha --config Debug
ctest --test-dir build/adasdf_cl-v0_6_alpha -C Debug --output-on-failure
```

## Build From Source

CMake options:

- `ADASDF_CL_BUILD_EXAMPLES`: build example executables.
- `ADASDF_CL_BUILD_TESTS`: build and register CTest tests.
- `ADASDF_CL_USE_EXISTING_CORE`: use the existing OctreeBlockLowRankSDF core when available.
- `ADASDF_CL_ENABLE_ADAPTIVE_BUILDER`: enable the public adaptive builder API.
- `ADASDF_CL_ENABLE_SURROGATE_RECOMMENDER`: enable the placeholder DASH-SDF recommender API.
- `ADASDF_CL_ENABLE_CUDA`: reserve CUDA hooks; CUDA is not required.

The library can configure without the existing core. In that mode, real `.sdfbin` loading and STL adaptive building return clear unavailable-backend errors, and tests that need the core skip gracefully.

## Install and Use From CMake

The 0.6.0-alpha tree installs headers, static library targets, and a package config skeleton:

```bash
cmake --install build --prefix install
```

Downstream CMake preview:

```cmake
find_package(AdaSDFCL CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE AdaSDFCL::adasdf_cl)
```

Full downstream install-tree validation is planned for v0.7.

## Build an Adaptive SDF From STL

```bash
build/adasdf_cl-v0_6_alpha/Debug/adasdf_build.exe adasdf_cl/tests/data/cube_closed_ascii.stl build/cube.sdfbin --near-surface-error 1e-4 --max-memory-mb 256 --compress
```

The CLI validates output by reloading the generated `.sdfbin`.

## Query Signed Distance and Normal

```cpp
#include <adasdf/adasdf.h>

auto model = adasdf::SDFBinReader::read("cube.sdfbin");
adasdf::Vector3 p{0.5, 0.5, 0.5};

double phi = model->sampleDistance(p);
adasdf::Vector3 grad = model->sampleGradient(p);
adasdf::Vector3 normal = model->sampleNormal(p);
```

Gradients currently use central finite differences over the loaded SDF query.

## FCL-Style Pair Collision

```cpp
#include <adasdf/adasdf.h>

auto model = adasdf::SDFBinReader::read("cube.sdfbin");

adasdf::CollisionObject a(model);
adasdf::CollisionObject b(model);
b.setTransform(adasdf::Transform::fromTranslation({0.25, 0.0, 0.0}));

adasdf::CollisionRequest request;
request.enable_contact = true;
request.max_contacts = 4;
request.query_mode = adasdf::QueryMode::Balanced;

adasdf::CollisionResult result;
bool hit = adasdf::collide(a, b, request, result);
```

The current CPU narrow-phase performs approximate symmetric SDF-sampling contact generation with deterministic contact reduction. Result objects report candidate counts, raw contact counts, reduced contact counts, SDF query counts, backend info, and method info.

## Command-Line Tools

Working:

- `adasdf_build`: build an STL into `.sdfbin` and validate reload.

Planned:

- `adasdf_info`
- `adasdf_query`
- `adasdf_collide`
- `adasdf_benchmark`

See `tools/README.md`.

## Working Examples

- `02_load_sdfbin_and_query.cpp`
- `03_collision_between_two_objects.cpp`
- `05_fcl_style_api.cpp`
- `06_build_then_query.cpp`
- `07_contact_reduction_demo.cpp`

Preview-only examples:

- `01_build_adaptive_sdf.cpp`
- `04_batched_gpu_query.cpp`

See `examples/README.md`.

## DASH-SDF Module

`SurrogateRecommender` is a placeholder API. It does not load a model or fabricate predictions when no backend is connected. Validation-card metrics from the external DASH-SDF work are documented for context only.

See `docs/surrogate_recommendation.md` and `docs/validation_card.md`.

## Validation Card

The current validation records:

- 17/17 CTest tests passed in the v0.6 local validation path.
- Cube STL build, `.sdfbin` round-trip, point query, pair collision, and contact reduction examples run successfully when the existing core is available.
- `scripts/check_repo_clean.py` passes after documentation and generated-file hygiene checks.

See `docs/alpha_status.md` and `docs/adasdf_cl_v0_6_alpha_release_hardening_report.md`.

## Limitations

- Pair collision is an approximate SDF-sampling narrow-phase.
- Distance for overlapping AABBs is an approximate sampled signed distance, not a strict global closest distance.
- Contact reduction is a deterministic heuristic, not a manifold optimizer.
- CUDA batched pair query is not implemented.
- FCL ABI compatibility is not provided.
- The surrogate recommender is a placeholder unless a future backend is explicitly connected.
- Contact-only `.sdfbin` remains planned.

See `docs/limitations.md`.

## Roadmap

- v0.6 alpha release hardening.
- v0.7 install/package and external project integration.
- v0.8 Python binding preview.
- v0.9 CUDA batched query preview.
- v1.0 stable research release.

## Citation

See `CITATION.cff`. The alpha citation metadata is a placeholder and does not claim a DOI.

## License

MIT. See `LICENSE`.

## Contributing

See `CONTRIBUTING.md`. Contributions should include tests where practical, avoid large generated artifacts, and document current limitations honestly.
