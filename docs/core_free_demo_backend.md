# Core-Free Demo Backend

AdaSDF-CL v0.8.0-alpha adds a small analytic SDF backend that works without the existing research core, CUDA, FCL, Python, vcpkg, or private paths.

The goal is a clone-only public workflow:

```text
cmake build/install
-> adasdf_make_demo_box
-> adasdf_info
-> adasdf_query
-> adasdf_collide
-> downstream CMake project
```

## What It Is

The backend is an analytic axis-aligned box SDF. It supports:

- signed distance;
- analytic gradient / normal;
- local AABB;
- memory footprint reporting;
- `SDFModel` query backend metadata;
- CPU narrow-phase collision through the existing AdaSDF-CL query API.

The backend reports:

```text
core-free analytic demo SDF backend
```

## Demo SDFBin Format

The demo `.sdfbin` format is a small text file:

```text
ADASDF_DEMO_SDFBIN_V1
shape box
center 0 0 0
half_extent 0.5 0.5 0.5
unit m
```

`SDFBinReader::read()` checks this magic before trying the existing-core reader. `SDFBinWriter::write()` writes this format when the model is an `AnalyticSDFModel`.

## Generate a Demo Box

```bash
adasdf_make_demo_box cube_demo.sdfbin
adasdf_make_demo_box cube_demo.sdfbin --center 0 0 0 --half-extent 0.5 0.5 0.5 --unit m
```

## Query

```bash
adasdf_info cube_demo.sdfbin
adasdf_query cube_demo.sdfbin --point 0 0 0
```

The default box has center distance `-0.5`, surface distance near `0`, and positive distance outside.

## Collide

```bash
adasdf_collide cube_demo.sdfbin cube_demo.sdfbin --offset 0.25 0 0 --max-contacts 4
```

This runs the same `CpuNarrowPhase` path used by the public API. Output includes collision state, minimum distance, backend, method, contact count, and first contact details.

## C++ API

```cpp
#include <adasdf/adasdf.h>

auto model = adasdf::AnalyticSDFModel::createBox();
auto phi = model->sampleDistance({0.0, 0.0, 0.0});
auto normal = model->sampleNormal({0.5, 0.0, 0.0});
```

## Difference From Full Adaptive SDFBin

The demo backend is not the final adaptive compressed SDF backend.

- It supports analytic boxes, not arbitrary meshes.
- Its `.sdfbin` format is a readable demo format, not the existing compressed adaptive format.
- It is meant for public demos, external package tests, and API learning.
- Full STL-to-sdfbin adaptive construction still requires the existing core or future standalone builder work.

Generated demo `.sdfbin` files should stay in build, install, or temporary directories and should not be committed to the source tree.
