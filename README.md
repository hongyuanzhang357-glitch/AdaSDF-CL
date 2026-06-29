# AdaSDF-CL

Adaptive Signed Distance Field Collision Library

Status: 0.8.0-alpha / research preview
Build system: CMake
License: MIT
Tests: CTest

AdaSDF-CL is an alpha collision and contact library built around signed distance fields. It provides an FCL-style API for distance, collision, and contact queries while keeping CUDA, FCL, Python, and surrogate-model integrations optional.

v0.8.0-alpha introduces a core-free analytic demo SDF backend. It allows users to run a complete SDF query and collision demo without the existing research core. Full adaptive compressed SDF construction still requires the existing core or future standalone builder work.

## Core Features

- Core-free analytic box SDF model for public demo and integration tests.
- Demo `.sdfbin` reader/writer using the text magic `ADASDF_DEMO_SDFBIN_V1`.
- `adasdf_make_demo_box`, `adasdf_info`, `adasdf_query`, and `adasdf_collide` CLI workflow that runs without existing core dependencies.
- Adaptive SDF builder bridge for STL input through the existing OctreeBlockLowRankSDF core when that optional core is available.
- `.sdfbin` reader and writer round-trip for existing-core-backed models.
- `SDFModel` point signed-distance, gradient, and normal queries.
- FCL-style `CollisionObject`, `CollisionRequest`, `CollisionResult`, `DistanceRequest`, and `DistanceResult`.
- CPU approximate SDF-sampling narrow-phase with symmetric candidate queries and deterministic contact reduction.
- Installable CMake package consumed with `find_package(AdaSDFCL CONFIG REQUIRED)`.
- Validation-card documentation for the DASH-SDF surrogate work, with runtime recommender kept as a placeholder.

## v0.8 Quick Start

```bash
git clone https://github.com/hongyuanzhang357-glitch/AdaSDF-CL.git
cd AdaSDF-CL
git checkout v0.8.0-alpha

cmake -S . -B build -DADASDF_CL_BUILD_EXAMPLES=ON -DADASDF_CL_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release --prefix install

install/bin/adasdf_make_demo_box cube_demo.sdfbin
install/bin/adasdf_info cube_demo.sdfbin
install/bin/adasdf_query cube_demo.sdfbin --point 0 0 0
install/bin/adasdf_collide cube_demo.sdfbin cube_demo.sdfbin --max-contacts 4
```

On Windows, installed tools usually have `.exe` suffixes, for example `install/bin/adasdf_make_demo_box.exe`.

The generated `cube_demo.sdfbin` is a small public demonstration file. Do not commit generated `.sdfbin` files to the source tree.

## Backend Status

AdaSDF-CL currently has two practical modes.

### Core-Free Public Build

The public repository can be configured, built, tested, installed, and consumed by external CMake projects without the original research core.

This mode supports:

- analytic box SDF query and gradient sampling;
- demo `.sdfbin` generation, reading, point query, and pair collision;
- CMake install/export;
- `find_package(AdaSDFCL CONFIG REQUIRED)`;
- public headers and API preview;
- CLI tool installation;
- external compile/link smoke tests.

The demo `.sdfbin` format is for public demonstration and integration learning. It is not the full adaptive compressed SDF format.

### Existing-Core Enhanced Build

When configured with `ADASDF_CL_USE_EXISTING_CORE=ON` and a valid `ADASDF_CL_EXISTING_ROOT`, AdaSDF-CL can use the existing research core to:

- build compressed adaptive `.sdfbin` files from STL;
- read existing-core `.sdfbin` assets;
- query signed distance and gradient;
- run CPU SDF-sampling pair collision.

This path is functional in the author's local environment but is not yet a fully self-contained public distribution. Downstream users may need to provide the existing core dependency prefix, such as vcpkg-installed dependencies.

## Build From Source

CMake options:

- `ADASDF_CL_BUILD_EXAMPLES`: build example executables.
- `ADASDF_CL_BUILD_TESTS`: build and register CTest tests.
- `ADASDF_CL_ENABLE_DEMO_BACKEND`: enable the core-free analytic demo backend, default `ON`.
- `ADASDF_CL_USE_EXISTING_CORE`: use the existing OctreeBlockLowRankSDF core when available.
- `ADASDF_CL_ENABLE_ADAPTIVE_BUILDER`: enable the public adaptive builder API.
- `ADASDF_CL_ENABLE_SURROGATE_RECOMMENDER`: enable the placeholder DASH-SDF recommender API.
- `ADASDF_CL_ENABLE_CUDA`: reserve CUDA hooks; CUDA is not required.

The library can configure without the existing core. In that mode, demo `.sdfbin` files work, while full adaptive STL-to-sdfbin construction returns clear unavailable-backend errors.

## Install and Use From CMake

The 0.8.0-alpha tree installs headers, static library targets, command-line tools, license/readme metadata, and CMake package files:

```bash
cmake --install build --config Release --prefix install
```

Downstream CMake preview:

```cmake
find_package(AdaSDFCL CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE AdaSDFCL::adasdf_cl)
```

The exported `AdaSDFCL::adasdf_cl` target links the runtime library. A separate `AdaSDFCL::adasdf_cl_headers` target is exported for header-only inspection.

Standalone install validation:

```bash
python scripts/run_install_validation.py --source . --build ../build/adasdf_cl_install_validation --install ../build/adasdf_cl_install --config Release
```

See `docs/external_integration.md` and `examples/downstream_cmake_project/README.md`.

## Demo SDFBin Format

v0.8.0-alpha adds a lightweight text format for clone-only public demos:

```text
ADASDF_DEMO_SDFBIN_V1
shape box
center 0 0 0
half_extent 0.5 0.5 0.5
unit m
```

`SDFBinReader::read()` detects this magic first and returns an `AnalyticSDFModel`. If the magic is not present, it falls back to the existing-core reader. `SDFBinWriter::write()` writes this demo format for analytic models.

## Query Signed Distance and Normal

Core-free demo query:

```cpp
#include <adasdf/adasdf.h>

auto model = adasdf::AnalyticSDFModel::createBox();
adasdf::Vector3 p{0.0, 0.0, 0.0};

double phi = model->sampleDistance(p);
adasdf::Vector3 grad = model->sampleGradient(p);
adasdf::Vector3 normal = model->sampleNormal(p);
```

For existing-core `.sdfbin` assets, use `SDFBinReader::read("model.sdfbin")` in a build configured with the existing core.

## FCL-Style Pair Collision

Core-free demo collision:

```cpp
#include <adasdf/adasdf.h>

auto model = adasdf::AnalyticSDFModel::createBox();

adasdf::CollisionObject a(model);
adasdf::CollisionObject b(model);
b.setTransform(adasdf::Transform::fromTranslation({0.25, 0.0, 0.0}));

adasdf::CollisionRequest request;
request.enable_contact = true;
request.max_contacts = 4;

adasdf::CollisionResult result;
bool hit = adasdf::collide(a, b, request, result);
```

The current CPU narrow-phase performs approximate symmetric SDF-sampling contact generation with deterministic contact reduction. Result objects report candidate counts, raw contact counts, reduced contact counts, SDF query counts, backend info, and method info.

## Command-Line Tools

Working:

- `adasdf_make_demo_box`: create a core-free demo box `.sdfbin`.
- `adasdf_build`: build an STL into `.sdfbin` when the existing adaptive builder bridge is available.
- `adasdf_info`: print loaded `.sdfbin` metadata.
- `adasdf_query`: sample one point from a loaded `.sdfbin`.
- `adasdf_collide`: run a two-model collision query.

Planned:

- `adasdf_benchmark`

See `tools/README.md`.

## Working Examples

- `02_load_sdfbin_and_query.cpp`
- `03_collision_between_two_objects.cpp`
- `05_fcl_style_api.cpp`
- `06_build_then_query.cpp`
- `07_contact_reduction_demo.cpp`
- `08_core_free_demo_collision.cpp`
- `downstream_cmake_project`: package-only external CMake smoke example with a no-input core-free demo path.

Preview-only examples:

- `01_build_adaptive_sdf.cpp`
- `04_batched_gpu_query.cpp`

See `examples/README.md`.

## Validation Card

The current validation records:

- CTest passes in the source build path.
- Install validation passes for an existing-core-free package build and external downstream CMake consumer.
- Core-free `adasdf_make_demo_box`, `adasdf_info`, `adasdf_query`, and `adasdf_collide` pass on a generated demo `.sdfbin`.
- Core-free demo collision example passes.
- Cube STL build, full adaptive `.sdfbin` round-trip, and existing-core collision examples remain available only when the existing core is configured.
- `scripts/check_repo_clean.py` passes after documentation and generated-file hygiene checks.

See `docs/alpha_status.md`, `docs/core_free_demo_backend.md`, `docs/github_release_draft_v0_8_0_alpha.md`, and `docs/limitations.md`.

## Limitations

- The v0.8 demo backend supports analytic boxes, not arbitrary meshes.
- The demo `.sdfbin` format is not the full adaptive compressed SDF format.
- Full adaptive STL-to-sdfbin construction still requires the existing core or future standalone builder work.
- Pair collision is an approximate SDF-sampling narrow-phase.
- Distance for overlapping AABBs is an approximate sampled signed distance, not a strict global closest distance.
- Contact reduction is a deterministic heuristic, not a manifold optimizer.
- CUDA batched pair query is not implemented.
- FCL ABI compatibility is not provided.
- Python bindings are not implemented.
- The surrogate recommender is a placeholder unless a future backend is explicitly connected.

See `docs/limitations.md`.

## Roadmap

- v0.8 core-free standalone demo backend.
- v0.9 Python binding preview.
- v1.0 CUDA/FCL/native backend maturation targets remain research-roadmap work.

## Citation

See `CITATION.cff`. The alpha citation metadata is a placeholder and does not claim a DOI.

## License

MIT. See `LICENSE`.

## Contributing

See `CONTRIBUTING.md`. Contributions should include tests where practical, avoid large generated artifacts, and document current limitations honestly.
